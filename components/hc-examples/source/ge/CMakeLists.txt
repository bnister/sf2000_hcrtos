set(CMAKE_BUILD_TYPE "DEBUG")

add_definitions("-O0 -g")

function(add_test arg1)
	add_executable(${arg1} ${arg1}.c ${arg2})
	add_dependencies(${arg1} ${PROJECT_NAME})
	target_link_libraries(${arg1} ${PROJECT_NAME} m)
	install(PROGRAMS ${arg1} DESTINATION bin)
endfunction()

function(add_test_p2 arg1 arg2)
	add_executable(${arg1} ${arg1}.c ${arg2})
	add_dependencies(${arg1} ${PROJECT_NAME})
	target_link_libraries(${arg1} ${PROJECT_NAME} m)
	install(PROGRAMS ${arg1} DESTINATION bin)
endfunction()


add_test("hcge_fill_rect_test")
add_test("hcge_blit_test")
add_test("hcge_rotate_test")
add_test_p2("hcge_cust_api_test" "customer_api/mi_gfx.c")

#add_executable(hcge_fill_rect_test hcge_fill_rect_test.c)
#add_executable(hcge_blit_test hcge_blit_test.c)
#add_executable(hcge_blit_test hcge_blit_test.c)

#add_dependencies(hcge_fill_rect_test ${PROJECT_NAME})
#add_dependencies(hcge_blit_test ${PROJECT_NAME})

#target_link_libraries(hcge_fill_rect_test ${PROJECT_NAME} m)
#target_link_libraries(hcge_blit_test ${PROJECT_NAME} m)

#install(PROGRAMS hcge_fill_rect_test DESTINATION bin)
#install(PROGRAMS hcge_blit DESTINATION bin)
