# middle-end-optis

Compiler middle-end optimizations

## dead-code-elim (C++ / LLVM)

Dead Code Elimination.  
Remove instructions unused / without side effects, empty and unreachable blocks
Engineer a Compiler Book.

## dominance (C++ / LLVM)

Dominance Sets.  
Build the dominance sets of every basic blocks.  
Engineer a Compiler Book.

## dom-value-numbering (C++)

Dominator Based value numbering technique.  
Find duplicate instructions and replace it with regs of result already computed.  
Found duplicate accross multiple basic blocks using block from parents in DOM tree.  
Input code is SSA.  
Engineer a Compiler Book.

## fun-inliner (C++)

Function inlining.  
Inline all function calls.  
Heursitic choice not implemented.  
Engineer a Compiler Book.

## global-code-placement (C++)

Global Code Placement.  
Reorder basic blocks in a mroe efficient manner.  
Try to order them in order to reduce jumps for the more executed blocks.  
Use profilling infos (in comments in example files).  
Engineer a Compiler Book.

## idom (C++)

Immediate Dominator.  
Build Dominator tree and Immediate Dominator / Dominator sets.  
Engineer a Compiler Book.

## interproc-constprop (C++)

Interprocedular Constant Propagation.  
Constant Propagation through call sites.  
If all call sites use same constant value for an argument, can be propogated to function body.  
Input code is SSA.  
Engineer a Compiler Book.

## lazy-code-motion (C++)

Lazy Code Motion.  
Try to move code in early blocks where it gets executed less often.  
Implementation doesn't work well.  
Engineer a Compiler Book.

## live-uninit-regs (C++)

Live Unitialized Registers.  
Find registers that are potentially used before being defined.  
Engineer a Compiler Book.

## local-value-numbering (C++)

Local Value Numbering.  
Find duplicate instructions in a basic block using hash tables.  
Engineer a Compiler Book.

## procedure-placement (C++)

Procedure Placement.  
Reorder procedures in close.  
Try to put calees close to callers, to improve cache performance.  
Choices based on heursitics.  
Engineer a Compiler Book.

## sparsecond-constprop (C++)

Sparse Conditional Constant Propagation.  
Perform constant propagation through cycles, only for block that get executed
(ignore branches never taken).
SSA Code.  
Engineer a Compiler Book.

## sparse-simple-constprop (C++)

Sparse Simple Constant Propagation.  
Perform constant propagation on SSA.  
This version can propagate values through cycles of the SSA graph.  
SSA Code.  
Engineer a Compiler Book.

## ssa-semipruned (C++)

Conversion to SSA semi-pruned formed.  
Convert non-SSA code into SSA semi-pruned (doesn't remove all useless phis).  
Engineer a Compiler Book.

## superblock-cloning (C++)

Superblock Cloning.  
Clone all blocks in all paths from loop begin.  
This way, some jumps to a single block may be eliminated (reduce branches).  
SSA form.  
Engineer a Compiler Book.

## superlocal-value-numbering (C++)

Superlocal Value Numbering.  
Extension to Local Value Numbering, that applyes to a path a basic blocks, 
each having only one predecessor.
Engineer a Compiler Book.

## tree-height-balancing (C++ / LLVM)

Tree Height Balancing.  
Find in each block sequences of operations that correspondong to a tree of integer operations.  
Rebalance the tree, which should reduce the instruction dependencies.  
Engineer a Compiler Book.

