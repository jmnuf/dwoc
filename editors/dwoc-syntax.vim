" Vim syntax file
" Language:	Dwo Code
"
" This is a minimal syntax file, and it will try to remain simple
" Was today years old when I learned that "Red Hat Linux DWARF Debugging Information" is a .dwo file hence why .dwoc is here
" 

syn match   DwoCodeComment      "\/\/.*"
syn keyword DwoCodeKeyword      fn let use
" IDK got an LLM to generate the number regex, I hate touching regexes
syn match DwoCodeNumber         "\<\d\+\(_\d\+\)*\(\.\(\d\+\(_\d\+\)*\)\?\)\?\>"

" Define the default highlighting.
hi def link DwoCodeComment		Comment
hi def link DwoCodeKeyword		Keyword
hi def link DwoCodeNumber              Number


" Reminder on getting filetype detection
" autocmd BufNewFile,BufRead *.dwo,*.dwoc set filetype=dwo-code

" Reminder on loading syntax highlighting
" syntax enable
" runtime! syntax/dwoc-syntax.vim


