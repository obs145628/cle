# reg-alloc

Register Allocation algorithms

## block-bottomup (C++)

Toy algorithm implementation.  
Allocates reg for a single block, through one pass that alloc / free / spill regs.  
Enginneer a Compiler book.

## block-naive (C++)

Toy algorithm implementation.  
Allocates reg for a single block, assigning each virt reg one hard reg for the whole block.  
Enginneer a Compiler book.

## color-ssa-td (C++)

Toy algorithm implementation.  
Allocates reg for a whole function in SSA form.  
Based on Graph coloring, Top-Down approach.  
Really naive approach, suppose all regs are general purpose.
