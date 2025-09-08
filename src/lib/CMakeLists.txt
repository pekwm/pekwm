cmake_minimum_required(VERSION 3.10)

set(lib_SOURCES
    Calendar.cc
    CfgParser.cc
    CfgParserKey.cc
    CfgParserSource.cc
    CfgParserVarExpander.cc
    CfgParserVarExpanderX11.cc
    Charset.cc
    CMakeLists.txt
    Compat.cc
    Cond.cc
    Daytime.cc
    Debug.cc
    Geometry.cc
    HttpClient.cc
    Json.cc
    Location.cc
    Md5.cc
    Observable.cc
    Os.cc
    RegexString.cc
    String.cc
    Timeouts.cc
    Tokenizer.cc
    Util.cc
    X11.cc
    X11_XRandr.cc)

add_library(lib STATIC ${lib_SOURCES})
target_include_directories(lib PUBLIC
			   ${PROJECT_BINARY_DIR}/src
			   ${common_INCLUDE_DIRS})
target_compile_definitions(lib PUBLIC PEKWM_SH="${SH}")
