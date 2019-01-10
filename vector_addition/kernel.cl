__kernel void vector_addition(__global int * A, __global int * B, __global int * C) {
	// get the index of the next element to add together.
	int ind = get_global_id(0);
	// our work item size is 1, so each global id corresponds to one sum we need to do
	C[ind] = A[ind] + B[ind];
}
