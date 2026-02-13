cmake_minimum_required(VERSION 4.0)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(PetukhPlusPlus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES arm64)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")

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
        src/semantics/SemanticAnalyzer.h
        src/semantics/SemanticAnalyzer.cpp

        src/rpn/RPNGenerator.cpp
        src/vm/Value.h
        src/vm/VM.h
        src/vm/VM.cpp
)

if(APPLE)
    target_link_libraries(PetukhPlusPlus PRIVATE c++)
endif()
