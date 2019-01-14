__kernel void vector_addition(__global int * A, __global int * B, __global int * C) {
	// get the index of the next element (work item) to add together.
	int ind = get_global_id(0);
	C[ind] = A[ind] + B[ind];
}
