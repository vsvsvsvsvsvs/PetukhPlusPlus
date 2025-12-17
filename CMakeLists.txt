cmake_minimum_required(VERSION 4.0)
project(PetukhPlusPlus)

set(CMAKE_CXX_STANDARD 20)

add_executable(PetukhPlusPlus
        src/main.cpp

        src/lexer/Token.h

        src/lexer/Lexer.h
        src/lexer/Lexer.cpp

        src/lexer/Trie.h
        src/lexer/Trie.cpp

        src/parser/AST.h

        src/parser/Parser.h
        src/parser/Parser.cpp

        src/parser/ASTPrinter.h
        src/parser/ASTPrinter.cpp
)
