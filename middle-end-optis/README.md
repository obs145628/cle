# middle-end-optis

Compiler middle-end optimizations

## dom-value-numbering (C++)

Dominator Based value numbering technique.  
Find duplicate instructions and replace it with regs of result already computed.  
Found duplicate accross multiple basic blocks using block from parents in DOM tree.  
Input code is SSA.  
Engineer a Compiler Book.

