# CSCAT:a cross-platform struct compatibility auto-fix tool for static binary translation

CACAT is a cross-platform struct compatibility auto-fix tool (CSCAT) for static binary translation.

the input of this tool is an elf file, and the output is a list of different struct which this elf file contian on two platfroms. 


this tool has been tested to work on x86-64 Ubuntu20.04.

# Dependencies
to use this tool,you have to install :

| Name                                                       | Version |
| ---------------------------------------------------------- | ------- |
| [jsoncpp](https://github.com/open-source-parsers/jsoncpp)                                | Latest  |
| [llvm](https://github.com/llvm/llvm-project)                                | 20+   |
| [Clang](http://clang.llvm.org/)                            | 20+   |
| [pyelftools](https://github.com/eliben/pyelftools)           | Latest  |


