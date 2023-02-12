cmake_minimum_required(VERSION 3.5)

set(lib_SOURCES
    CfgParser.cc
    CfgParserKey.cc
    CfgParserSource.cc
    CfgParserVarExpander.cc
    CfgParserVarExpanderX11.cc
    Charset.cc
    CMakeLists.txt
    Compat.cc
    Debug.cc
    Observable.cc
    RegexString.cc
    String.cc
    Tokenizer.cc
    Util.cc
    X11.cc)

add_library(lib STATIC ${lib_SOURCES})
target_include_directories(lib PUBLIC ${common_INCLUDE_DIRS})
target_compile_definitions(lib PUBLIC PEKWM_SH="${SH}")
