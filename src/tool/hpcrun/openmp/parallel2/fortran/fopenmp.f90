subroutine fn(count)
        integer(8) :: count
	count = count + 1
end subroutine fn

subroutine loop(count)
        integer(8) :: count
	integer(8) :: i
        integer(8) :: n
        n = ishft(1, 29)
	do i = 0, n-1 
           call fn(count)
        enddo
end subroutine loop


program main
        integer(8) :: count
!omp parallel num_threads(2)
        call loop(count)
end program
