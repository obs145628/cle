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
