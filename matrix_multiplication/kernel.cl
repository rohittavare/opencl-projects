__kernel void matrix_multiplication( __global int * a, __global int * b, __global int * c, __global int * dim) {
	int i = get_global_id(0);
	int j = get_global_id(1);
	int k = 0;
	int l;
	for(l = 0; l < dim[1]; l++) {
		k += a[i * dim[0] + l] * b[l * dim[2] + j];	
	}
	c[dim[2] * i + j] = k;
}
