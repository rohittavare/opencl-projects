__kernel void vector_addition(__global int * A, __global int * B, __global int * C) {
	int i = get_global_id(0);
	C[i] = A[i] + B[i];
}

__kernel void vector_multiplication(__global int * A, __global int * B, global int * C) {
	int i = get_global_id(0);
	C[i] = A[i] * B[i];
}
