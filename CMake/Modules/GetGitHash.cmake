function(get_git_hash)
	find_program(GIT git)
	if(NOT GIT)
		message(WARNING "git not found")
		return()
	endif()

	execute_process(
		COMMAND ${GIT} rev-parse HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		RESULT_VARIABLE GIT_RESULT
	)

	if(NOT GIT_RESULT EQUAL 0)
		message(WARNING "failed to get git hash")
		return()
	endif()

	set(GIT_HASH ${GIT_HASH} PARENT_SCOPE)
endfunction()

get_git_hash()
