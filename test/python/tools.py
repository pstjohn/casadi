#
#     This file is part of CasADi.
# 
#     CasADi -- A symbolic framework for dynamic optimization.
#     Copyright (C) 2010 by Joel Andersson, Moritz Diehl, K.U.Leuven. All rights reserved.
# 
#     CasADi is free software; you can redistribute it and/or
#     modify it under the terms of the GNU Lesser General Public
#     License as published by the Free Software Foundation; either
#     version 3 of the License, or (at your option) any later version.
# 
#     CasADi is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     Lesser General Public License for more details.
# 
#     You should have received a copy of the GNU Lesser General Public
#     License along with CasADi; if not, write to the Free Software
#     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
# 
# 
from casadi import *
import casadi as c
from numpy import *
import unittest
from types import *
from helpers import *
from casadi.tools import *

class Toolstests(casadiTestCase):

  def test_collection(self):
      self.message("Collection")

      p = Collection()
      p.x = ssym("x",2)
      p.z = ssym("z",2,4)
      p.y = ssym("y",3,2)

      p.freeze()
      
      self.assertTrue(isinstance(p[...],SXMatrix))
      self.assertEqual(p.size,16)
      self.assertEqual(p[...].shape[0],16)
      self.assertEqual(p[...].shape[1],1)

      self.assertTrue(isEqual(p.x,p["x"]))
      self.assertTrue(isEqual(p.y,p["y"]))
      self.assertTrue(isEqual(p.z,p["z"]))

      self.checkarray(p.i_x,IMatrix([0,1]),"")
      self.checkarray(p.i_z,IMatrix([[2,4,6,8],[3,5,7,9]]),"")
      self.checkarray(p.i_y,IMatrix([[10,13],[11,14],[12,15]]),"")
      
      self.checkarray(p.iv_x,list(IMatrix([0,1])),"")
      
      p = Collection()
      p.x = [ssym("x")]
      p.z = [ssym("z%d" % i) for i in range(2)]
      p.y = [[ssym("y%d%d"% (i,j)) for i in range(2)] for j in range(3)]
      p.setOrder(["x","y","z"])
      p.freeze()
      
      self.assertTrue(isinstance(p[...],SXMatrix))
      self.assertEqual(p.size,9)

      self.assertTrue(all(map(isEqual,p.x,p["x"])))
      self.assertTrue(all(map(isEqual,p.z,p["z"])))
      self.assertTrue(all(map(lambda a,b: all(map(isEqual,a,b)),p.y,p["y"])))
      self.checkarray(p[...],vertcat([p.x[0],p.y[0][0],p.y[0][1],p.y[1][0],p.y[1][1],p.y[2][0],p.y[2][1],p.z[0],p.z[1]]),"")

      self.checkarray(p.i_x[0],0,"")
      self.checkarray(p.i_z[0],7,"")
      self.checkarray(p.i_z[1],8,"")
      self.checkarray(p.i_y[0],[1,2],"")
      self.checkarray(p.i_y[1],[3,4],"")
      
      self.checkarray(p.i_y.__getitem__(0),[1,2],"")
      #self.checkarray(p.i_y.__getitem__((0,)),[1,2],"")

      self.checkarray(p.i_y,[[1,2],[3,4],[5,6]],"")
      
      #self.checkarray(p.i_y[:,0],[1,3,5],"")
      #self.checkarray(p.i_y[:,1],[2,4,6],"")
      
      #self.checkarray(p.i_y[0,:],[1,2],"")
      #self.checkarray(p.i_y[1,:],[3,4],"")
      
      #self.checkarray(p._i["y",0],[1,2],"")
      #self.checkarray(p._i["y",:,0],[1,3,5],"")
      
      p = Collection()
      p.x = [ssym("x")]
      p.z = [ssym("z%d" % i) for i in range(2)]
      p.y = [[ssym("y%d%d"% (i,j)) for i in range(2)] for j in range(3)]
      p.setOrder(["x",("y","z")])
      p.freeze()
      
      self.assertTrue(isinstance(p[...],SXMatrix))
      self.assertEqual(p.size,9)

      self.assertTrue(all(map(isEqual,p.x,p["x"])))
      self.assertTrue(all(map(isEqual,p.z,p["z"])))
      self.assertTrue(all(map(lambda a,b: all(map(isEqual,a,b)),p.y,p["y"])))
      self.checkarray(p[...],vertcat([p.x[0],p.y[0][0],p.y[0][1],p.z[0],p.y[1][0],p.y[1][1],p.z[1],p.y[2][0],p.y[2][1]]),"")

      self.checkarray(p.z[...],vertcat([p.z[0],p.z[1]]),"")
      self.checkarray(p.y[...],vertcat([p.y[0][0],p.y[0][1],p.y[1][0],p.y[1][1],p.y[2][0],p.y[2][1]]),"")
      
      self.checkarray(p.i_x[0],0,"")
      self.checkarray(p.i_z[0],3,"")
      self.checkarray(p.i_z[1],6,"")
      self.checkarray(p.i_z[...],IMatrix([3,6]),"")
      self.checkarray(p.i_y[0],[1,2],"")
      self.checkarray(p.i_y[1],[4,5],"")
      self.checkarray(p.i_y[2],[7,8],"")
      self.checkarray(p.i_y[...],IMatrix([1,2,4,5,7,8]),"")
    
      p = Collection()
      p.a = ssym("a")
      p.b = ssym("b")
      p.freeze()

      g = Collection()
      g.c = ssym("c")
      g.d = p
      g.e = ssym("e")
      g.freeze()
       
      self.assertEqual(g.size,4)
      self.checkarray(g[...],vertcat([g.c,p.a,p.b,g.e]),"")

      self.assertEqual(p.size,2)
      self.checkarray(p[...],vertcat([p.a,p.b]),"")
      
      self.checkarray(p.i_a,0)
      self.checkarray(p.i_b,1)

      self.checkarray(g.i_c,0)
      self.checkarray(g.i_d["a"],1)
      self.checkarray(g.i_d["b"],2)
      self.checkarray(g.i_e,3)
      
      self.checkarray(g.d[...],vertcat([p.a,p.b]),"")
      

  def test_variables(self):
      self.message("Variables")
             
      p = Variables()

      p.x = ssym("x",2)
      p.z = ssym("z",2,4)
      p.y = ssym("y",3,2)

      xother = Variables()
      xother.a = SX("x")
      xother.b = diag(ssym("[a,b]"))
      xother.freeze()
      
      p.xother = xother
      
      p.freeze()

      self.assertEqual(p.o_x,0)
      self.assertEqual(p.o_xother,2)
      self.assertEqual(p.xother.o_a,0)
      self.assertEqual(p.xother.o_b,1)
      self.assertEqual(p.o_y,5)
      self.assertEqual(p.o_z,11)
      
      self.assertEqual(p.I_x,0,"Index")
      self.assertEqual(p.I_y,2,"Index")
      self.assertEqual(p.I_z,3,"Index")
      self.assertEqual(p.xother.I_a,0,"Index")
      self.assertEqual(p.xother.I_b,1,"Index")
      
      self.checkarray(array(p.i_x),DMatrix([[0],[1]]),"index")
      self.checkarray(array(p.i_y),DMatrix([[5,8],[6,9],[7,10]]),"index")
      self.checkarray(array(p.i_z),DMatrix([[11,13,15,17],[12,14,16,18]]),"index")
      self.checkarray(array(p.xother.i_a),DMatrix(0),"index")
      self.checkarray(array(p.xother.i_b),DMatrix([[1,0],[0,2]]),"index")

      self.assertEqual(p.veccat().numel(),21)
      self.assertEqual(p.veccat().size(),19)
      self.assertEqual(p.vecNZcat().numel(),19)
      self.assertEqual(p.vecNZcat().size(),19)
      self.assertEqual(p.getNumel(),21)
      self.assertEqual(p.getSize(),19)
      
      self.assertTrue(p.lookup(('x',)) is p.x)
      self.assertTrue(p.lookup(('x',(0,))).toScalar().isEqual(p.x[0].toScalar()))
      self.assertTrue(p.lookup(('x',(1,))).toScalar().isEqual(p.x[1].toScalar()))
      self.assertTrue(p.lookup(('y',)) is p.y)
      for i in range(6):
        self.assertTrue(p.lookup(('y',(i,))).toScalar().isEqual(p.y[i].toScalar()))
      for i in range(3):
        for j in range(2):
          self.assertTrue(p.lookup(('y',(i,j))).toScalar().isEqual(p.y[i,j].toScalar()))
      self.assertTrue(p.lookup(('z',)) is p.z)
      self.assertTrue(p.lookup(('xother',)) is p.xother)
      self.assertTrue(p.lookup(('xother','a')) is p.xother.a)
      self.assertTrue(p.lookup(('xother','a',(0,))).isEqual(p.xother.a))
      self.assertTrue(p.lookup(('xother','b')) is p.xother.b)
      self.assertTrue(p.lookup(('xother','b',(1,1))).toScalar().isEqual(p.xother.b[1].toScalar()))
      
      
      for k in range(p.getSize()):
        roundtrip = p.lookup(p.reverselookup(k))
        if not(isinstance(roundtrip,SX)):
          roundtrip = roundtrip.toScalar()
        self.assertTrue(roundtrip.isEqual(p.vecNZcat()[k].toScalar()))
      
      p.x_ = [5,8]

      p.xother.b_.setAll(7)
      p.z_ = DMatrix([[1,2,3,4],[5,6,7,8]])


      A = p.vecNZcat_()
      
      self.checkarray(A,DMatrix([5,8,0,7,7,0,0,0,0,0,0,1,5,2,6,3,7,4,8]),"vecNZcat")
      
      A = p.veccat_()
      
      self.checkarray(A,DMatrix([5,8,0,7,0,0,7,0,0,0,0,0,0,1,5,2,6,3,7,4,8]),"veccat")
      
      self.checkarray(A[p.i_z],p.z_,"indexing round trip")
      self.checkarray(A[p.o_xother + p.xother.i_b],p.xother.b_,"indexing round trip 2")

      p = Variables()
      p.a = ssym("a",2)
      b = []
      b.append(ssym("b1",3))
      b.append(ssym("b2",3))
      p.b = b
      p.c = ssym("c")
      p.freeze()
      

      self.assertEqual(p.veccat().numel(),9)
      self.assertEqual(p.veccat().size(),9)
      self.assertEqual(p.vecNZcat().numel(),9)
      self.assertEqual(p.vecNZcat().size(),9)
      self.assertEqual(p.getNumel(),9)
      self.assertEqual(p.getSize(),9)
      
      self.checkarray(array(p.i_a),DMatrix([[0],[1]]),"index")
      self.checkarray(array(p.i_b[0]),DMatrix([[2],[3],[4]]),"index")
      self.checkarray(array(p.i_b[1]),DMatrix([[5],[6],[7]]),"index")
      self.checkarray(array(p.i_c),DMatrix(8),"index")

      self.assertEqual(p.o_a,0,"Offset")
      self.assertEqual(p.o_b[0],2,"Offset")
      self.assertEqual(p.o_b[1],5,"Offset")
      self.assertEqual(p.o_c,8,"Offset")
      
      self.assertEqual(p.I_a,0,"Index")
      self.assertEqual(p.I_b[0],1,"Index")
      self.assertEqual(p.I_b[1],2,"Index")
      self.assertEqual(p.I_c,3,"Index")
      
      
      self.assertTrue(p.lookup(('b',)) is p.b)
      self.assertTrue(p.lookup(('b',0)) is p.b[0])
      self.assertTrue(p.lookup(('b',1)) is p.b[1])
      
      for i in range(3):
        self.assertTrue(p.lookup(('b',1,(i,))).toScalar().isEqual(p.b[1][i].toScalar()))

            
      for k in range(p.getSize()):
        roundtrip = p.lookup(p.reverselookup(k))
        if not(isinstance(roundtrip,SX)):
          roundtrip = roundtrip.toScalar()
        self.assertTrue(roundtrip.isEqual(p.vecNZcat()[k].toScalar()))
        
      p.b_[1].setAll(4)
      
      A = p.vecNZcat_()
 
      self.checkarray(A,DMatrix([0,0,0,0,0,4,4,4,0]),"vecNZcat")
     
      p.b_[0].setAll(3)
      A = p.veccat_()

      self.checkarray(A,DMatrix([0,0,3,3,3,4,4,4,0]),"vecNZcat")
      
      
      p = Variables()
      p.a = ssym("a",2)
      b = []
      b.append(ssym("b1",3,2))
      b.append(ssym("b2",3))
      p.b = b
      p.c = ssym("c")
      p.freeze()
      

      
      self.checkarray(array(p.i_a),DMatrix([[0],[1]]),"index")
      self.checkarray(array(p.i_b[0]),DMatrix([[2,5],[3,6],[4,7]]),"index")
      self.checkarray(array(p.i_b[1]),DMatrix([[8],[9],[10]]),"index")
      self.checkarray(array(p.i_c),DMatrix(11),"index")

      self.assertEqual(p.o_a,0,"Offset")
      self.assertEqual(p.o_b[0],2,"Offset")
      self.assertEqual(p.o_b[1],8,"Offset")
      self.assertEqual(p.o_c,11,"Offset")
      
      self.assertEqual(p.I_a,0,"Index")
      self.assertEqual(p.I_b[0],1,"Index")
      self.assertEqual(p.I_b[1],2,"Index")
      self.assertEqual(p.I_c,3,"Index")
      
      p.b_[1].setAll(4)
      
      A = p.vecNZcat_()
 
      self.checkarray(A,DMatrix([0,0,0,0,0,0,0,0,4,4,4,0]),"vecNZcat")
     
      p.b_[0].setAll(3)
      p.b_[0][2,0] = 9
      A = p.veccat_()

      self.checkarray(A,DMatrix([0,0,3,3,9,3,3,3,4,4,4,0]),"vecNZcat")

      p = Variables()
      p.a = msym("a",2)
      b = []
      b.append(msym("b1",3))
      b.append(msym("b2",3))
      p.b = b
      p.c = msym("c")
      p.freeze()
      
      f = MXFunction([p.veccat()],[p.a,p.b[0],p.b[1],p.c])
      f.init()
      
      f.input()[p.i_a]=[4,5]
      
      f.evaluate()
      
      p = Variables()
      p.a = ssym("a",2)
      b = []
      b.append(ssym("b1",3))
      b.append(ssym("b2",3))
      b.append([ssym("b3",3),ssym("b4",3)])
      p.b = b
      p.c = ssym("c")
      p.freeze()
      
      self.checkarray(array(p.i_a),DMatrix([[0],[1]]),"index")
      self.checkarray(array(p.i_b[0]),DMatrix([[2],[3],[4]]),"index")
      self.checkarray(array(p.i_b[1]),DMatrix([[5],[6],[7]]),"index")
      self.checkarray(array(p.i_b[2][0]),DMatrix([[8],[9],[10]]),"index")
      self.checkarray(array(p.i_b[2][1]),DMatrix([[11],[12],[13]]),"index")
      self.checkarray(array(p.i_c),DMatrix(14),"index")

      self.assertEqual(p.o_a,0,"Offset")
      self.assertEqual(p.o_b[0],2,"Offset")
      self.assertEqual(p.o_b[1],5,"Offset")
      self.assertEqual(p.o_b[2][0],8,"Offset")
      self.assertEqual(p.o_b[2][1],11,"Offset")
      self.assertEqual(p.o_c,14,"Offset")
      
      self.assertEqual(p.I_a,0,"Index")
      self.assertEqual(p.I_b[0],1,"Index")
      self.assertEqual(p.I_b[1],2,"Index")
      self.assertEqual(p.I_b[2][0],3,"Index")
      self.assertEqual(p.I_b[2][1],4,"Index")
      self.assertEqual(p.I_c,5,"Index")

      for k in range(p.getSize()):
        roundtrip = p.lookup(p.reverselookup(k))
        if not(isinstance(roundtrip,SX)):
          roundtrip = roundtrip.toScalar()
        self.assertTrue(roundtrip.isEqual(p.vecNZcat()[k].toScalar()))
        
        
      x = msym("a",2,3)
      
      p = Variables()
      p.a = x**2
      p.b = [ x**3, sqrt(x)]
      p.c = [1/x]
      p.freeze(False)
      
      self.assertTrue(p.a.isBinary())
      
  def test_Numbers(self):
      p = Variables()

      p.x = ssym("x",2)
      p.z = ssym("z",2,4)
      p.y = ssym("y",3,2)

      xother = Variables()
      xother.a = SX("x")
      xother.b = diag(ssym("[a,b]"))
      xother.freeze()
      
      p.xother = xother
      
      p.freeze()
      
      p_ = Numbers(p)
      
      p_.y.setAll(3)
      
      p_.x = [4,5]
      
      p_.xother.a=12

      self.checkarray(p_.vecNZcat(),DMatrix([4,5,12,0,0,3,3,3,3,3,3,0,0,0,0,0,0,0,0]),"vecNZcat")

if __name__ == '__main__':
    unittest.main()

