
aux_source_directory(. SRC)

# 编译不过的，先放这里
set(NOT_COMPILE )

foreach(FILE_PATH ${SRC})
	string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FILE_PATH})
	list(FIND NOT_COMPILE ${FILE_NAME} RESULT)
	if(${RESULT} EQUAL -1)
		message(${FILE_NAME})
		add_executable(${FILE_NAME} ${FILE_PATH})
		target_link_libraries(${FILE_NAME}  nty_core ${LIBS})
	endif()
endforeach()
