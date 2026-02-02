" Lang3 syntax highlighting for Vim

if exists("b:current_syntax")
  finish
endif

highlight default link Lang3Keyword Keyword
highlight default link Lang3Boolean Boolean
highlight default link Lang3Constant Constant
highlight default link Lang3Operator Operator
highlight default link Lang3Number Number
highlight default link Lang3String String
highlight default link Lang3Escape SpecialChar
highlight default link Lang3Function Function
highlight default link Lang3Delimiter Delimiter
highlight default link Lang3Comment Comment

syntax match Lang3Comment "\v#.*$"
syntax keyword Lang3Keyword if then else elif while for in step do return break continue fn let mut end
syntax keyword Lang3Boolean true false
syntax keyword Lang3Constant nil
syntax keyword Lang3Operator or and not
syntax match Lang3Operator "\v(\+\=|\-\=|\*\=|\/\=|\%\=|\^\=|\.\.\=|\=\=|\!\=|\<\=|\>\=|\.\.)"
syntax match Lang3Operator "[\+\-\*/%\=\<\>\.]"
syntax match Lang3Operator "[\(\)\{\}\[\];,]"
syntax match Lang3Function "\v\w+\ze\("
syntax match Lang3Number "\v\d+"
syntax region Lang3String start='"' end='"' contains=Lang3Escape
syntax match Lang3Escape contained "\\[nt\\"]"

let b:current_syntax = "lang3"
