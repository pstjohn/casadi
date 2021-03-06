/*
*    This file is part of CasADi.
*
*    CasADi -- A symbolic framework for dynamic optimization.
*    Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
*
*    CasADi is free software; you can redistribute it and/or
*    modify it under the terms of the GNU Lesser General Public
*    License as published by the Free Software Foundation; either
*    version 3 of the License, or (at your option) any later version.
*
*    CasADi is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    Lesser General Public License for more details.
*
*    You should have received a copy of the GNU Lesser General Public
*    License along with CasADi; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
*/

#include "mx_function_internal.hpp"
#include "../mx/evaluation_mx.hpp"
#include "../mx/mapping.hpp"
#include "../mx/mx_tools.hpp"
#include "../sx/sx_tools.hpp"

#include "../stl_vector_tools.hpp"
#include "../casadi_types.hpp"

#include <stack>
#include <typeinfo>

// To reuse variables we need to be able to sort by sparsity pattern (preferably using a hash map)
#ifdef USE_CXX11
// Using C++11 unordered_map (hash map)
#include <unordered_map>
#define SPARSITY_MAP std::unordered_map
#else // USE_CXX11
// Falling back to std::map (binary search tree)
#include <map>
#define SPARSITY_MAP std::map
#endif // USE_CXX11

using namespace std;

namespace CasADi{

MXFunctionInternal::MXFunctionInternal(const std::vector<MX>& inputv, const std::vector<MX>& outputv) :
  XFunctionInternal<MXFunction,MXFunctionInternal,MX,MXNode>(inputv,outputv) {
  
  setOption("name", "unnamed_mx_function");
  setOption("numeric_jacobian", true);
  setOption("numeric_hessian", true);
  
  // Check if any inputs is a mapping
  bool has_mapping_inputs = false;
  for(vector<MX>::const_iterator it = inputv_.begin(); it!=inputv_.end(); ++it){
    has_mapping_inputs = has_mapping_inputs || it->isMapping();
  }
  
  // If one or more inputs is a mapping, elimination needed before creating function
  if(has_mapping_inputs){
    
    // Name of replaced variable
    stringstream rep_name;
    int ind = 0;

    // Temp vectors
    std::vector<int> inz, onz;
    
    // Find new variables and expressions for all inputs that needs to be eliminated
    std::vector<MX> v, vdef;
    for(vector<MX>::iterator it = inputv_.begin(); it!=inputv_.end(); ++it, ++ind){
      if(it->isMapping()){
        
        // Get the mapping node
        Mapping* n = dynamic_cast<Mapping*>(it->get());
        
        // Initialize the mapping, i.e. sort by input and output index
        n->init();
        
        // Create a new variable
        rep_name.clear();
        rep_name << "r_" << ind;
        MX new_var = msym(rep_name.str(),n->sparsity());
        
        // For all dependencies to be replaced
        for(int iind=0; iind<n->ndep(); ++iind){
          
          // Variable to be replaced
          MX v_dep = n->dep(iind);
          casadi_assert_message(v_dep.isSymbolic(),"Mapping inputs may only map to symbolic variables.");
          
          // Express this variable in terms of the new variable
          const vector<pair<int,int> >& assigns = n->index_output_sorted_[0][iind];
          inz.clear();
          onz.clear();
          for(vector<pair<int,int> >::const_iterator it_ass=assigns.begin(); it_ass!=assigns.end(); ++it_ass){
            inz.push_back(it_ass->second);
            onz.push_back(it_ass->first);
          }
          MX vdef_dep = MX::create(new Mapping(v_dep.sparsity()));
          vdef_dep->assign(new_var,inz,onz);
          
          // Save variable to list of variables to be replaced
          v.push_back(v_dep);
          vdef.push_back(vdef_dep);
        }
        
        // Replace variable
        *it = new_var;
      }
    }
    
    // Replace expressions
    outputv_ = substitute(outputv_,v,vdef);
  }
  
  // Check for duplicate entries among the input expressions
  bool has_duplicates = false;
  for(vector<MX>::iterator it = inputv_.begin(); it != inputv_.end(); ++it){
    has_duplicates = has_duplicates || it->getTemp()!=0;
    it->setTemp(1);
  }
  
  // Reset temporaries
  for(vector<MX>::iterator it = inputv_.begin(); it != inputv_.end(); ++it){
    it->setTemp(0);
  }
  casadi_assert_message(!has_duplicates, "The input expressions are not independent.");
  
  liftfun_ = 0;
  liftfun_ud_ = 0;
}


MXFunctionInternal::~MXFunctionInternal(){
}


void MXFunctionInternal::init(){
  log("MXFunctionInternal::init begin");
      
  // Call the init function of the base class
  XFunctionInternal<MXFunction,MXFunctionInternal,MX,MXNode>::init();    

  // Stack used to sort the computational graph
  stack<MXNode*> s;

  // All nodes
  vector<MXNode*> nodes;
  
  // Add the list of nodes
  int ind=0;
  for(vector<MX>::iterator it = outputv_.begin(); it != outputv_.end(); ++it, ++ind){
    // Add outputs to the list
    s.push(static_cast<MXNode*>(it->get()));
    sort_depth_first(s,nodes);
    
    // A null pointer means an output instruction
    nodes.push_back(static_cast<MXNode*>(0));
  }
  
  // Make sure that all inputs have been added also // TODO REMOVE THIS
  for(vector<MX>::iterator it = inputv_.begin(); it != inputv_.end(); ++it){
    if(!it->getTemp()){
      nodes.push_back(static_cast<MXNode*>(it->get()));
    }
  }
  
  // Set the temporary variables to be the corresponding place in the sorted graph
  for(int i=0; i<nodes.size(); ++i){
    if(nodes[i]){
      nodes[i]->temp = i;
    }
  }

  // Place in the algorithm for each node
  vector<int> place_in_alg;
  place_in_alg.reserve(nodes.size());
  
  // Use live variables?
  bool live_variables = getOption("live_variables");
  
  // Input instructions
  vector<pair<int,MXNode*> > symb_loc;

  // Current output and nonzero, start with the first one
  int curr_oind=0;

  // Count the number of times each node is used
  vector<int> refcount(nodes.size(),0);

  // Get the sequence of instructions for the virtual machine
  algorithm_.resize(0);
  algorithm_.reserve(nodes.size());
  for(vector<MXNode*>::iterator it=nodes.begin(); it!=nodes.end(); ++it){
    // Current node
    MXNode* n = *it;
 
    // Get the operation
    int op = n==0 ? OP_OUTPUT : n->getOp();
    
    // Store location if parameter (or input)
    if(op==OP_PARAMETER){
      symb_loc.push_back(make_pair(algorithm_.size(),n));
    }
    
    // If a new element in the algorithm needs to be added
    if(op>=0){
      AlgEl ae;
      ae.op = op;
      ae.data.assignNode(n);
    
      // Add input and output argument
      if(op==OP_OUTPUT){
        ae.arg.resize(1);
        ae.arg[0] = outputv_.at(curr_oind)->temp;
        ae.res.resize(1);
        ae.res[0] = curr_oind++;
      } else {
        ae.arg.resize(n->ndep());
        for(int i=0; i<n->ndep(); ++i){
          ae.arg[i] = n->dep(i).isNull() ? -1 : n->dep(i)->temp;
        }
        ae.res.resize(n->getNumOutputs());
        if(n->isMultipleOutput()){
          fill(ae.res.begin(),ae.res.end(),-1);
        } else {
          ae.res[0] = n->temp;
        }
      }
      
      // Increase the reference count of the dependencies
      for(int c=0; c<ae.arg.size(); ++c){
        if(ae.arg[c]>=0){
          refcount[ae.arg[c]]++;
        }
      }
       
      // Save to algorithm
      place_in_alg.push_back(algorithm_.size());
      algorithm_.push_back(ae);
      
    } else { // Function output node
      // Get the output index
      int oind = n->getFunctionOutput();

      // Get the index of the parent node
      int pind = place_in_alg[n->dep(0)->temp];
      
      // Save location in the algorithm element corresponding to the parent node
      int& otmp = algorithm_[pind].res.at(oind);
      if(otmp<0){
        otmp = n->temp; // First time this function output is encountered, save to algorithm
      } else {
        n->temp = otmp; // Function output is a duplicate, use the node encountered first
      }
      
      // Not in the algorithm
      place_in_alg.push_back(-1);
    }
  }

  // Place in the work vector for each of the nodes in the tree (overwrites the reference counter)
  vector<int>& place = place_in_alg; // Reuse memory as it is no longer needed
  place.resize(nodes.size());
  
  // Stack with unused elements in the work vector, sorted by sparsity pattern
  SPARSITY_MAP<const void*,stack<int> > unused_all;
  
  // Work vector size
  int worksize = 0;
  
  // Find a place in the work vector for the operation
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it){
    
    // Allocate/reuse memory for the results of the operation
    if(it->op!=OP_OUTPUT){
      // Allocate new variables
      for(int c=0; c<it->res.size(); ++c){
        if(it->res[c]>=0){
          
          // Are reuse of variables (live variables) enabled?
          if(live_variables){
            // Get a pointer to the sparsity pattern node
            const void* sp = it->data->sparsity(c).get();
            
            // Get a reference to the stack for the current sparsity
            stack<int>& unused = unused_all[sp];
            
            // Try to reuse a variable from the stack if possible (last in, first out)
            if(!unused.empty()){
              it->res[c] = place[it->res[c]] = unused.top();
              unused.pop();
              continue; // Success, no new element needed in the work vector
            }
          }
      
          // Allocate a new element in the work vector
          it->res[c] = place[it->res[c]] = worksize++;
        }
      }
    }
    
    // Dereference or free the memory of the arguments
    for(int c=it->arg.size()-1; c>=0; --c){ // reverse order so that the first argument will end up at the top of the stack

      // Index of the argument
      int& ch_ind = it->arg[c];
      if(ch_ind>=0){
        
        // Decrease reference count and add to the stack of unused variables if the count hits zero
        int remaining = --refcount[ch_ind];
        
        // Free variable for reuse
        if(live_variables && remaining==0){
          
          // Get a pointer to the sparsity pattern of the argument that can be freed
          const void* sp = nodes[ch_ind]->sparsity().get();
          
          // Add to the stack of unused work vector elements for the current sparsity
          unused_all[sp].push(place[ch_ind]);
        }
        
        // Point to the place in the work vector instead of to the place in the list of nodes
        ch_ind = place[ch_ind];
      }
    }
  }
  
  if(verbose()){
    if(live_variables){
      cout << "Using live variables: work array is " <<  worksize << " instead of " << nodes.size() << endl;
    } else {
      cout << "Live variables disabled." << endl;
    }
  }
  
  // Allocate work vectors (numeric)
  work_.resize(0);
  work_.resize(worksize);
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it){
    if(it->op!=OP_OUTPUT){
      for(int c=0; c<it->res.size(); ++c){
        if(it->res[c]>=0 && work_[it->res[c]].data.empty()){
          work_[it->res[c]].data = Matrix<double>(it->data->sparsity(c),0);
        }
      }
    }
  }
  
  // Reset the temporary variables
  for(int i=0; i<nodes.size(); ++i){
    if(nodes[i]){
      nodes[i]->temp = 0;
    }
  }
  
  // Now mark each input's place in the algorithm
  for(vector<pair<int,MXNode*> >::const_iterator it=symb_loc.begin(); it!=symb_loc.end(); ++it){
    it->second->temp = it->first+1;
  }
  
  // Add input instructions
  for(int ind=0; ind<inputv_.size(); ++ind){
    int i = inputv_[ind].getTemp()-1;
    if(i>=0){
      // Mark as input
      algorithm_[i].op = OP_INPUT;
      
      // Location of the input
      algorithm_[i].arg = vector<int>(1,ind);
      
      // Mark input as read
      inputv_[ind].setTemp(0);
    }
  }
  
  // Locate free variables
  free_vars_.clear();
  for(vector<pair<int,MXNode*> >::const_iterator it=symb_loc.begin(); it!=symb_loc.end(); ++it){
    int i = it->second->temp-1;
    if(i>=0){
      // Save to list of free parameters
      free_vars_.push_back(MX::create(it->second));
      
      // Remove marker
      it->second->temp=0;
    }
  }
  
  // Allocate tape
  allocTape();
  
  
  // Allocate memory for directional derivatives
  MXFunctionInternal::updateNumSens(false);

  log("MXFunctionInternal::init end");
}

void MXFunctionInternal::updateNumSens(bool recursive){
  // Call the base class if needed
  if(recursive) XFunctionInternal<MXFunction,MXFunctionInternal,MX,MXNode>::updateNumSens(recursive);
  
  // Allocate work for directional derivatives
  for(vector<FunctionIO>::iterator it=work_.begin(); it!=work_.end(); it++){
    it->dataF.resize(nfdir_,it->data);
    it->dataA.resize(nadir_,it->data);
  }
}

void MXFunctionInternal::setLiftingFunction(LiftingFunction liftfun, void* user_data){
  liftfun_ = liftfun;
  liftfun_ud_ = user_data;
}

void MXFunctionInternal::updatePointers(const AlgEl& el, int nfdir, int nadir){
  mx_input_.resize(el.arg.size());
  mx_output_.resize(el.res.size());

  mx_fwdSeed_.resize(nfdir);
  mx_fwdSens_.resize(nfdir);
  for(int d=0; d<nfdir; ++d){
    mx_fwdSeed_[d].resize(mx_input_.size());
    mx_fwdSens_[d].resize(mx_output_.size());
  }

  mx_adjSens_.resize(nadir);
  mx_adjSeed_.resize(nadir);
  for(int d=0; d<nadir; ++d){
    mx_adjSens_[d].resize(mx_input_.size());
    mx_adjSeed_[d].resize(mx_output_.size());
  }
  
  for(int i=0; i<mx_input_.size(); ++i){
    if(el.arg[i]>=0){
      mx_input_[i] = &work_[el.arg[i]].data;
      for(int d=0; d<nfdir; ++d) mx_fwdSeed_[d][i] = &work_[el.arg[i]].dataF[d];
      for(int d=0; d<nadir; ++d) mx_adjSens_[d][i] = &work_[el.arg[i]].dataA[d];
    } else {
      mx_input_[i] = 0;
      for(int d=0; d<nfdir; ++d) mx_fwdSeed_[d][i] = 0;
      for(int d=0; d<nadir; ++d) mx_adjSens_[d][i] = 0;
    }
  }
  
  for(int i=0; i<mx_output_.size(); ++i){
    if(el.res[i]>=0){
      mx_output_[i] = &work_[el.res[i]].data;
      for(int d=0; d<nfdir; ++d) mx_fwdSens_[d][i] = &work_[el.res[i]].dataF[d];
      for(int d=0; d<nadir; ++d) mx_adjSeed_[d][i] = &work_[el.res[i]].dataA[d];
    } else {
      mx_output_[i] = 0;
      for(int d=0; d<nfdir; ++d) mx_fwdSens_[d][i] = 0;
      for(int d=0; d<nadir; ++d) mx_adjSeed_[d][i] = 0;
    }
  }
}

void MXFunctionInternal::evaluate(int nfdir, int nadir){
  log("MXFunctionInternal::evaluate begin");

  // Make sure that there are no free variables
  if (!free_vars_.empty()) {
    std::stringstream ss;
    repr(ss);
    casadi_error("Cannot evaluate \"" << ss.str() << "\" since variables " << free_vars_ << " are free.");
  }
  
  // Tape iterator
  vector<pair<pair<int,int>,DMatrix> >::iterator tape_it = tape_.begin();
  
  // Evaluate all of the nodes of the algorithm: should only evaluate nodes that have not yet been calculated!
  int alg_counter = 0;
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it, ++alg_counter){
    // Spill existing work elements if needed
    if(nadir>0 && it->op!=OP_OUTPUT){
      for(vector<int>::const_iterator c=it->res.begin(); c!=it->res.end(); ++c){
        if(*c >=0 && tape_it!=tape_.end() && tape_it->first == make_pair(alg_counter,*c)){
          tape_it->second.set(work_[*c].data);
          tape_it++;
        }
      }
    }
    
    if(it->op==OP_INPUT){
      // Pass the input and forward seeeds
      work_[it->res.front()].data.set(input(it->arg.front()));
      for(int dir=0; dir<nfdir; ++dir){
        work_[it->res.front()].dataF.at(dir).set(fwdSeed(it->arg.front(),dir));
      }
    } else if(it->op==OP_OUTPUT){
      // Get the outputs and forward sensitivities
      work_[it->arg.front()].data.get(output(it->res.front()));
      for(int dir=0; dir<nfdir; ++dir){
        work_[it->arg.front()].dataF.at(dir).get(fwdSens(it->res.front(),dir));
      }
    } else if(it->op==OP_PARAMETER){
      //casadi_error("The algorithm contains free parameters"); // FIXME
    } else {

      // Point pointers to the data corresponding to the element
      updatePointers(*it,nfdir,0);

      // Evaluate
      it->data->evaluateD(mx_input_, mx_output_, mx_fwdSeed_, mx_fwdSens_, mx_adjSeed_, mx_adjSens_);
  
      // Lifting
      if(liftfun_ && it->data->isNonLinear()){
        for(int i=0; i<it->res.size(); ++i){
          liftfun_(&mx_output_[i]->front(),mx_output_[i]->size(),liftfun_ud_);
        }
      }
    }
  }
  
  log("MXFunctionInternal::evaluate evaluated forward");
          
  if(nadir>0){
    log("MXFunctionInternal::evaluate evaluating adjoint");
    
    // Tape iterator (reuse same name to prevent usage of the forward iterator)
    vector<pair<pair<int,int>,DMatrix> >::const_reverse_iterator tape_it = tape_.rbegin();
    
    // Clear the adjoint seeds
    for(vector<FunctionIO>::iterator it=work_.begin(); it!=work_.end(); it++){
      for(int dir=0; dir<nadir; ++dir){
        fill(it->dataA.at(dir).begin(),it->dataA.at(dir).end(),0.0);
      }
    }

    // Evaluate all of the nodes of the algorithm: should only evaluate nodes that have not yet been calculated!
    int alg_counter = algorithm_.size()-1;
    for(vector<AlgEl>::reverse_iterator it=algorithm_.rbegin(); it!=algorithm_.rend(); ++it, --alg_counter){
      if(it->op==OP_INPUT){
        // Get the adjoint sensitivity
        for(int dir=0; dir<nadir; ++dir){
          work_[it->res.front()].dataA.at(dir).get(adjSens(it->arg.front(),dir));
        }
      } else if(it->op==OP_OUTPUT){
        // Pass the adjoint seeds
        for(int dir=0; dir<nadir; ++dir){
          const DMatrix& aseed = adjSeed(it->res.front(),dir);
          DMatrix& aseed_dest = work_[it->arg.front()].dataA.at(dir);
          transform(aseed_dest.begin(),aseed_dest.end(),aseed.begin(),aseed_dest.begin(),std::plus<double>());
        }
      } else if(it->op==OP_PARAMETER){
        //casadi_error("The algorithm contains free parameters"); // FIXME
      } else {
        // Point pointers to the data corresponding to the element
        updatePointers(*it,0,nadir);
        
        // Evaluate
        it->data->evaluateD(mx_input_, mx_output_, mx_fwdSeed_, mx_fwdSens_, mx_adjSeed_, mx_adjSens_);
      }
      
      if(it->op!=OP_OUTPUT){
        // Recover spilled work vector elements
        for(vector<int>::const_reverse_iterator c=it->res.rbegin(); c!=it->res.rend(); ++c){
          if(*c >=0 && tape_it!=tape_.rend() && tape_it->first==make_pair(alg_counter,*c)){
            tape_it->second.get(work_[*c].data);
            tape_it++;
          }
        }
        
        // Free memory for reuse
        for(int oind=0; oind<it->res.size(); ++oind){
          int el = it->res[oind];
          if(el>=0){
            for(int d=0; d<nadir; ++d){
              work_[el].dataA.at(d).setZero();
            }
          }
        }
      }
    }
    
    log("MXFunctionInternal::evaluate evaluated adjoint");
  }
  log("MXFunctionInternal::evaluate end");
}

void MXFunctionInternal::print(ostream &stream) const{
  FXInternal::print(stream);
  for(vector<AlgEl>::const_iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it){
    if(it->op==OP_OUTPUT){
      stream << "output[" << it->res.front() << "] = @" << it->arg.front();
    } else {
      if(it->res.size()==1){
        stream << "@" << it->res.front() << " = ";
      } else {
        stream << "{";
        for(int i=0; i<it->res.size(); ++i){
          if(i!=0) stream << ",";
          if(it->res[i]>=0){
            stream << "@" << it->res[i];
          } else {
            stream << "NULL";
          }
        }
        stream << "} = ";
      }
      if(it->op==OP_INPUT){
        stream << "input[" << it->arg.front() << "]";
      } else {
        it->data->printPart(stream,0);
        for(int i=0; i<it->arg.size(); ++i){
          if(it->arg[i]>=0){
            stream << "@" << it->arg[i];
          } else {
            stream << "NULL";
          }
          it->data->printPart(stream,i+1);
        }
      }
    }
    stream << endl;
  }
}

MXFunctionInternal* MXFunctionInternal::clone() const{
  return new MXFunctionInternal(*this);
}

void MXFunctionInternal::deepCopyMembers(std::map<SharedObjectNode*,SharedObject>& already_copied){
  XFunctionInternal<MXFunction,MXFunctionInternal,MX,MXNode>::deepCopyMembers(already_copied);
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it){
    if(it->op==OP_CALL){
      it->data.makeUnique(already_copied,false);
      it->data->getFunction() = deepcopy(it->data->getFunction(),already_copied);
    }
  }
}

void MXFunctionInternal::spInit(bool fwd){
  // Start by setting all elements of the work vector to zero
  for(vector<FunctionIO>::iterator it=work_.begin(); it!=work_.end(); ++it){
    //Get a pointer to the int array
    bvec_t *iwork = get_bvec_t(it->data.data());
    fill_n(iwork,it->data.size(),bvec_t(0));
  }
}

void MXFunctionInternal::spEvaluate(bool fwd){
  if(fwd){ // Forward propagation

    // Propagate sparsity forward
    for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); it++){
      if(it->op==OP_INPUT){
        // Pass input seeds
        vector<double> &w = work_[it->res.front()].data.data();
        bvec_t* iwork = get_bvec_t(w);
        bvec_t* swork = get_bvec_t(input(it->arg.front()).data());
        copy(swork,swork+w.size(),iwork);
      } else if(it->op==OP_OUTPUT){
        // Get the output sensitivities
        vector<double> &w = work_[it->arg.front()].data.data();
        bvec_t* iwork = get_bvec_t(w);
        bvec_t* swork = get_bvec_t(output(it->res.front()).data());
        copy(iwork,iwork+w.size(),swork);
      } else if(it->op==OP_PARAMETER){
        // Parameters are constant
        vector<double> &w = work_[it->res.front()].data.data();
        fill_n(get_bvec_t(w),w.size(),bvec_t(0));
      } else {
        // Point pointers to the data corresponding to the element
        updatePointers(*it,0,0);

        // Propagate sparsity forwards
        it->data->propagateSparsity(mx_input_, mx_output_,true);
      }
    }
    
  } else { // Backward propagation

    // Propagate sparsity backwards
    for(vector<AlgEl>::reverse_iterator it=algorithm_.rbegin(); it!=algorithm_.rend(); it++){
      if(it->op==OP_INPUT){
        // Get the input sensitivities and clear it from the work vector
        vector<double> &w = work_[it->res.front()].data.data();
        bvec_t* iwork = get_bvec_t(w);
        bvec_t* swork = get_bvec_t(input(it->arg.front()).data());
        for(int k=0; k<w.size(); ++k){
          swork[k] = iwork[k];
        }
      } else if(it->op==OP_OUTPUT){
        // Pass output seeds
        vector<double> &w = work_[it->arg.front()].data.data();
        bvec_t* iwork = get_bvec_t(w);
        bvec_t* swork = get_bvec_t(output(it->res.front()).data());
        for(int k=0; k<w.size(); ++k){
          iwork[k] |= swork[k];
        }
      } else if(it->op!=OP_PARAMETER){
        // Point pointers to the data corresponding to the element
        updatePointers(*it,0,0);
        
        // Propagate sparsity backwards
        it->data->propagateSparsity(mx_input_, mx_output_,false);
      }
      
      // Clear the seeds for the next sweep
      if(it->op!=OP_OUTPUT){
        for(int oind=0; oind<it->res.size(); ++oind){
          int el = it->res[oind];
          if(el>=0){
            vector<double> &w = work_[el].data.data();
            fill_n(get_bvec_t(w),w.size(),bvec_t(0));
          }
        }
      }
    }
  }
}

FX MXFunctionInternal::getNumericJacobian(int iind, int oind, bool compact, bool symmetric){
  // Create expressions for the Jacobian
  vector<MX> ret_out;
  ret_out.reserve(1+outputv_.size());
  ret_out.push_back(jac(iind,oind,compact,symmetric,false,true));
  ret_out.insert(ret_out.end(),outputv_.begin(),outputv_.end());
  
  // Return function
  return MXFunction(inputv_,ret_out);  
}

void MXFunctionInternal::evalMX(const std::vector<MX>& arg, std::vector<MX>& res, 
                                const std::vector<std::vector<MX> >& fseed, std::vector<std::vector<MX> >& fsens, 
                                const std::vector<std::vector<MX> >& aseed, std::vector<std::vector<MX> >& asens,
                                bool output_given){
  log("MXFunctionInternal::evalMX begin");
  assertInit();
  casadi_assert_message(arg.size()==getNumInputs(),"Wrong number of input arguments");
  if(output_given){
    casadi_assert_message(res.size()==getNumOutputs(),"Wrong number of output arguments");
  }
  
  // Symbolic work, non-differentiated
  vector<MX> swork(work_.size());
  log("MXFunctionInternal::evalMX allocated work vector");
  
  // Get the number of directions
  const int nfdir = fseed.size();
  const int nadir = aseed.size();

  // "Tape" with spilled variables
  vector<pair<pair<int,int>,MX> > tape;
  if(nadir>0){
    tape.resize(tape_.size());
    for(int k=0; k<tape.size(); ++k){
      tape[k].first = tape_[k].first;
    }
  }

  // Tape iterator
  vector<pair<pair<int,int>,MX> >::iterator tape_it = tape.begin();
  
  // Allocate outputs
  if(!output_given){
    res.resize(outputv_.size());
  }
  
  // Allocate forward sensitivities
  fsens.resize(nfdir);
  for(int d=0; d<nfdir; ++d){
    fsens[d].resize(outputv_.size());
  }
  
  // Allocate adjoint sensitivities
  asens.resize(nadir);
  for(int d=0; d<nadir; ++d){
    asens[d].resize(inputv_.size());
  }
  log("MXFunctionInternal::evalMX allocated return value");
  
  MXPtrV input_p, output_p;
  MXPtrVV fseed_p(nfdir), fsens_p(nfdir);
  MXPtrVV aseed_p(nadir), asens_p(nadir);
  MXPtrVV dummy_p;

  // Work vector, forward derivatives
  std::vector<std::vector<MX> > dwork(work_.size());
  fill(dwork.begin(),dwork.end(),std::vector<MX>(nfdir));
  log("MXFunctionInternal::evalMX allocated derivative work vector (forward mode)");
    
  // Loop over computational nodes in forward order
  int alg_counter = 0;
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it, ++alg_counter){
    // Spill existing work elements if needed
    if(nadir>0 && it->op!=OP_OUTPUT){
      for(vector<int>::const_iterator c=it->res.begin(); c!=it->res.end(); ++c){
        if(*c >=0 && tape_it!=tape.end() && tape_it->first == make_pair(alg_counter,*c)){
          tape_it->second = swork[*c];
          tape_it++;
        }
      }
    }
    
    if(it->op == OP_INPUT){
      // Fetch input
      swork[it->res.front()] = arg[it->arg.front()];
      for(int d=0; d<nfdir; ++d){
        dwork[it->res.front()][d] = fseed[d][it->arg.front()];
      }
    } else if(it->op==OP_OUTPUT){
      // Collect the results
      if(!output_given){
        res[it->res.front()] = swork[it->arg.front()];
      }

      // Collect the forward sensitivities
      for(int d=0; d<nfdir; ++d){
        fsens[d][it->res.front()] = dwork[it->arg.front()][d];
      }
    } else if(it->op==OP_PARAMETER){
      // Fetch parameter
      swork[it->res.front()] = it->data;
      for(int d=0; d<nfdir; ++d){
        dwork[it->res.front()][d] = MX();
      }
    } else {
    
      // Pointers to the arguments of the evaluation
      input_p.resize(it->arg.size());
      for(int i=0; i<input_p.size(); ++i){
        int el = it->arg[i]; // index of the argument
        input_p[i] = el<0 ? 0 : &swork[el];
      }
          
      // Pointers to the result of the evaluation
      output_p.resize(it->res.size());
      for(int i=0; i<output_p.size(); ++i){
        int el = it->res[i]; // index of the output
        output_p[i] = el<0 ? 0 : &swork[el];
      }

      // Copy answer of the evaluation, if known
      if(output_given){
        for(int oind=0; oind<output_p.size(); ++oind){
          if(output_p[oind]){
            *output_p[oind] = it->data.getOutput(oind);
          }
        }
        
        // Quick continue if no evaluation needed
        if(nfdir==0) continue;
      }

      // Forward seeds and sensitivities
      for(int d=0; d<nfdir; ++d){
        fseed_p[d].resize(it->arg.size());
        for(int iind=0; iind<it->arg.size(); ++iind){
          int el = it->arg[iind];
          fseed_p[d][iind] = el<0 ? 0 : &dwork[el][d];
          
          // Give zero seed if null
          if(el>=0 && dwork[el][d].isNull()){
            if(d==0){
              dwork[el][d] = MX::sparse(input_p[iind]->size1(),input_p[iind]->size2());
            } else {
              dwork[el][d] = dwork[el][0];
            }
          }
        }

        fsens_p[d].resize(it->res.size());
        for(int oind=0; oind<it->res.size(); ++oind){
          int el = it->res[oind];
          fsens_p[d][oind] = el<0 ? 0 : &dwork[el][d];
        }
      }

      // Call the evaluation function
      it->data->evaluateMX(input_p,output_p,fseed_p,fsens_p,dummy_p,dummy_p,output_given);
    }
  }
  
  // Loop over computational nodes in reverse order
  if(nadir>0){
    // Work vector, adjoint derivatives
    fill(dwork.begin(),dwork.end(),std::vector<MX>(nadir));
    log("MXFunctionInternal::evalMX allocated derivative work vector (adjoint mode)");
    
    // Tape iterator (reuse same name to prevent usage of the forward iterator)
    vector<pair<pair<int,int>,MX> >::const_reverse_iterator tape_it = tape.rbegin();
    
    int alg_counter = algorithm_.size()-1;
    for(vector<AlgEl>::reverse_iterator it=algorithm_.rbegin(); it!=algorithm_.rend(); ++it, --alg_counter){      
      if(it->op == OP_INPUT){
        // Collect the symbolic adjoint sensitivities
        for(int d=0; d<nadir; ++d){
          asens[d][it->arg.front()] = dwork[it->res.front()][d];
        }
      } else if(it->op==OP_OUTPUT){
        // Pass the adjoint seeds
        for(int d=0; d<nadir; ++d){
          if(dwork[it->arg.front()][d].isNull())
            dwork[it->arg.front()][d] = aseed[d][it->res.front()];
          else
            dwork[it->arg.front()][d] += aseed[d][it->res.front()];
        }
      } else if(it->op!=OP_PARAMETER){
        // Get the arguments of the evaluation
        input_p.resize(it->arg.size());
        for(int i=0; i<input_p.size(); ++i){
          int el = it->arg[i]; // index of the argument
          input_p[i] = el<0 ? 0 : &swork[el];
        }
            
        // Result of the evaluation
        output_p.resize(it->res.size());
        for(int i=0; i<output_p.size(); ++i){
          int el = it->res[i]; // index of the output
          output_p[i] = el<0 ? 0 : &swork[el];
        }

        // Sensitivity arguments
        for(int d=0; d<nadir; ++d){
          aseed_p[d].resize(it->res.size());
          for(int oind=0; oind<it->res.size(); ++oind){
            int el = it->res[oind];
            aseed_p[d][oind] = el<0 ? 0 : &dwork[el][d];
            
            // Provide a zero seed if no seed exists
            if(el>=0 && dwork[el][d].isNull()){
              dwork[el][d] = MX::sparse(swork[el].size1(),swork[el].size2());
            }
          }

          asens_p[d].resize(it->arg.size());
          for(int iind=0; iind<it->arg.size(); ++iind){
            int el = it->arg[iind];
            asens_p[d][iind] = el<0 ? 0 : &dwork[el][d];
            
            // Set sensitivities to zero if not yet used
            if(el>=0 && dwork[el][d].isNull()){
              dwork[el][d] = MX::sparse(swork[el].size1(),swork[el].size2());
            }
          }
        }

        // Call the evaluation function
        it->data->evaluateMX(input_p,output_p,dummy_p,dummy_p,aseed_p,asens_p,true);
      }
      
      if(it->op!=OP_OUTPUT){
        // Recover spilled work vector elements
        for(vector<int>::const_reverse_iterator c=it->res.rbegin(); c!=it->res.rend(); ++c){
          if(*c >=0 && tape_it!=tape.rend() && tape_it->first == make_pair(alg_counter,*c)){
            swork[*c] = tape_it->second;
            tape_it++;
          }
        }
        
        // Free memory for reuse
        for(int oind=0; oind<it->res.size(); ++oind){
          int el = it->res[oind];
          if(el>=0){
            for(int d=0; d<nadir; ++d){
              dwork[el][d] = MX();
            }
          }
        }
      }
    }
  }
  log("MXFunctionInternal::evalMX end");
}

void MXFunctionInternal::evalSX(const std::vector<SXMatrix>& input_s, std::vector<SXMatrix>& output_s, 
                                const std::vector<std::vector<SXMatrix> >& fwdSeed, std::vector<std::vector<SXMatrix> >& fwdSens, 
                                const std::vector<std::vector<SXMatrix> >& adjSeed, std::vector<std::vector<SXMatrix> >& adjSens,
                                bool output_given, int offset_begin, int offset_end){
  casadi_assert_message(fwdSens.empty(),"Not implemented");
  casadi_assert_message(adjSeed.empty(),"Not implemented");
  casadi_assert_message(offset_begin==0,"Not implemented");
  casadi_assert_message(offset_end==0,"Not implemented");
  
  // Create a work array
  vector<SXMatrix> swork(work_.size());
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); it++){
    if(it->op!=OP_OUTPUT){
      for(int i=0; i<it->res.size(); ++i){
        if (it->res[i]>=0)
          swork[it->res[i]] = SXMatrix(it->data->sparsity(i));
      }
    }
  }
  
  // Evaluate all of the nodes of the algorithm: should only evaluate nodes that have not yet been calculated!
  vector<SXMatrix*> sxarg;
  vector<SXMatrix*> sxres;
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); it++){
    if(it->op==OP_INPUT){
      // Pass the input
      swork[it->res.front()].set(input_s[it->arg.front()]);
    } else if(it->op==OP_OUTPUT){
      // Get the outputs
      swork[it->arg.front()].get(output_s[it->res.front()]);
    } else if(it->op==OP_PARAMETER){
      continue; // FIXME
    } else {
      sxarg.resize(it->arg.size());
      for(int c=0; c<sxarg.size(); ++c){
        int ind = it->arg[c];
        sxarg[c] = ind<0 ? 0 : &swork[ind];
      }
      sxres.resize(it->res.size());
      for(int c=0; c<sxres.size(); ++c){
        int ind = it->res[c];
        sxres[c] = ind<0 ? 0 : &swork[ind];
      }
      it->data->evaluateSX(sxarg,sxres);
    }
  }
}

SXFunction MXFunctionInternal::expand(const std::vector<SXMatrix>& inputvsx ){
  assertInit();
  
  // Create inputs with the same name and sparsity as the matrix valued symbolic inputs
  vector<SXMatrix> arg(inputv_.size());
  if(inputvsx.empty()){ // No symbolic input provided
    for(int i=0; i<arg.size(); ++i){
      arg[i] = ssym(inputv_[i]->getName(),inputv_[i].sparsity());
    }
  } else { // Use provided symbolic input
    // Make sure number of inputs matches
    casadi_assert(inputvsx.size()==inputv_.size());
    
    // Make sure that sparsity matches
    for(int i=0; i<inputvsx.size(); ++i){
      casadi_assert(inputvsx[i].sparsity() == inputv_[i].sparsity());
    }

    // Copy to argument vector
    copy(inputvsx.begin(),inputvsx.end(),arg.begin());
  }

  // Create output vector with correct sparsity
  vector<SXMatrix> res(outputv_.size());
  for(int i=0; i<res.size(); ++i){
    res[i] = SXMatrix(outputv_[i].sparsity());
  }
  
  // No sensitivities
  vector<vector<SXMatrix> > dummy;
  
  // Evaluate symbolically
  evalSX(arg,res,dummy,dummy,dummy,dummy,false);
  
  // Create function
  SXFunction f(arg,res);
  return f;
}

void MXFunctionInternal::printTape(ostream &stream){
  for(int k=0; k<tape_.size(); ++k){
    stream << "tape( algorithm index = " << tape_[k].first.first << ", work index = " << tape_[k].first.second << ") = " << tape_[k].second.data() << endl;
  }
}

void MXFunctionInternal::printWork(int nfdir, int nadir, ostream &stream){
  for(int k=0; k<work_.size(); ++k){
    stream << "work[" << k << "] = " << work_[k].data.data() << endl;
  }
  
  for(int d=0; d<nfdir; ++d){
    for(int k=0; k<work_.size(); ++k){
      stream << "fwork[" << d << "][" << k << "] = " << work_[k].dataF[d].data() << endl;
    }
  }
  
  for(int d=0; d<nadir; ++d){
    for(int k=0; k<work_.size(); ++k){
      stream << "awork[" << d << "][" << k << "] = " << work_[k].dataA[d].data() << endl;
    }
  }
}

void MXFunctionInternal::allocTape(){
  // Marker of elements in the work vector still in use when being overwritten
  vector<bool> in_use(work_.size(),false);
  
  // Remove existing entries in the tape
  tape_.clear();
  
  // Evaluate the algorithm, keeping track of variables that are in use
  int alg_counter = 0;
  for(vector<AlgEl>::iterator it=algorithm_.begin(); it!=algorithm_.end(); ++it, ++alg_counter){
    if(it->op!=OP_OUTPUT){
      // Loop over operation outputs, spill if necessary
      for(int c=0; c<it->res.size(); ++c){
        int ind = it->res[c];
        if(ind>=0){
          if(in_use[ind]){
            // Spill
            tape_.push_back(make_pair(make_pair(alg_counter,ind),DMatrix(it->data->sparsity(c))));
          } else {
            // Mark in use
            in_use[ind] = true;
          }
        }
      }
    }
  }
}



} // namespace CasADi

