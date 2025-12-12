cmake_minimum_required(VERSION 4.0)
project(PetukhPlusPlus)

set(CMAKE_CXX_STANDARD 20)

add_executable(PetukhPlusPlus
        main.cpp

        Token.h

        Lexer.h
        Lexer.cpp

        Trie.h
        Trie.cpp

        AST.h

        Parser.h
        Parser.cpp

        ASTPrinter.h
        ASTPrinter.cpp
)
