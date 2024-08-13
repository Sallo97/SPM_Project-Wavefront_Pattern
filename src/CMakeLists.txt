# Compiling sequential code
add_executable(sequential sequential.cpp utils/square_matrix.h utils/elem_info.h utils/constants.h utils/compute_elem.h)

#Compiling FastFlow code
add_executable(parallel_fastflow parallel_fastflow.cpp
        utils/square_matrix.h
        utils/elem_info.h
        utils/constants.h
        utils/compute_elem.h
        utils/ff_task.h
        utils/ff_diag_info.h)




#TODO ADD MPI Codep