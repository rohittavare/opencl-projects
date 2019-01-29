#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <CL/cl.h>

const int m1 = 10;
const int n1 = 10;
const int m2 = 10;
const int n2 = 10;

void error(cl_int err, char * message) {
	if(err != CL_SUCCESS) {
		fprintf(stderr, "error: %s\n", message);
		exit(1);
	}
}

int main(int argc, char ** argv) {
	
	printf("A x B = C\n");

	// create out data
	int ** a = (int **)malloc(m1 * sizeof(int *));
	int ** b = (int **)malloc(m2 * sizeof(int *));
	int ** c = (int **)malloc(m1 * sizeof(int *));

	int i;
	int j;
	printf("A: (%i x %i)\n", m1, n1);
	for(i = 0; i < m1; i++) {
		a[i] = (int *)malloc(n1 * sizeof(int));
		for(j = 0; j < n1; j++) {
			a[i][j] = (i + j) % n1; 
			printf("%i ", a[i][j]);
		}
		printf("\n");
	}
	printf("B: (%i x %i)\n", m2, n2);
	for(i = 0; i < m2; i++) {
		b[i] = (int *)malloc(n2 * sizeof(int));
		for(j = 0; j < n2; j++) {
			b[i][j] = (i + j) % m2;
			printf("%i ", b[i][j]);
		}
		printf("\n");
	}
	for(i = 0; i < m1; i++) {
		c[i] = (int *)malloc(n2 * sizeof(int));
	}

	// read in our kernel file
	char * source = (char *) malloc(300 * sizeof(char));
	int src_file = open("kernel.cl", O_RDONLY);
	size_t src_len = read(src_file, source, 300);
	
	cl_int err;
	
	// query our platform and its device
	cl_platform_id platform;
	cl_device_id device;
	
	err = clGetPlatformIDs( 1, &platform, NULL);
	error(err, "could not get platform ID");
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
	error(err, "could not get device ID");

	// create a context
	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	error(err, "could not create context");

	// create our queue
	cl_queue_properties props[3];
	props[0] = CL_QUEUE_PROPERTIES;
	props[1] = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
	props[2] = 0;

	cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, props, &err);
	error(err, "could not create command queue");

	// create the program object
	cl_program prog = clCreateProgramWithSource(context, 1, (const char **) &source, &src_len, &err);
	error(err, "could not create program fom source");
	err = clBuildProgram(prog, 1, &device, NULL, NULL, NULL);
	error(err, "could not build program");

	// create our buffers
	cl_mem A = clCreateBuffer(context, CL_MEM_READ_ONLY, m1 * n1 * sizeof(int), NULL, &err);
	error(err, "could not create buffer A");
	cl_mem B = clCreateBuffer(context, CL_MEM_READ_ONLY, m2 * n2 * sizeof(int), NULL, &err);
	error(err, "could not create buffer B");
	cl_mem M = clCreateBuffer(context, CL_MEM_READ_ONLY, 3 * sizeof(int), NULL, &err);
	error(err, "could not create buffer M");
	cl_mem C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, m1 * n2 * sizeof(int), NULL, &err);
	error(err, "could not create buffer C");

	// create our events list
	cl_event e[10];

	// copy in our buffers
	err = clEnqueueWriteBuffer(queue, M, CL_FALSE, 0, sizeof(int), &m1, 0, NULL, e);
	error(err, "could not enqueue buffer write to M (line 103)");
	err = clEnqueueWriteBuffer(queue, M, CL_FALSE, sizeof(int), sizeof(int), &n1, 0, NULL, e + 1);
	error(err, "could not enqueue buffer write to M (line 105)");
	err = clEnqueueWriteBuffer(queue, M, CL_FALSE, 2 * sizeof(int), sizeof(int), &n2, 0, NULL, e + 2);
	error(err, "could not enqueue buffer write to M (line 107)");
	
	size_t origin[3];
	origin[0] = 0;
	origin[1] = 0;
	origin[2] = 0;

	size_t region[3];
	region[0] = n1;
	region[1] = m1;
	region[2] = 1;

	size_t region2[3];
	region2[0] = n2;
	region2[1] = m2;
	region2[2] = 1;

	err = clEnqueueWriteBufferRect(queue, A, CL_FALSE, origin, origin, region, region[0] * sizeof(int), 0, region[0] * sizeof(int), 0, a, 0, NULL, e + 3);
	error(err, "could not enqueue rect buffer write from a to A");
	err = clEnqueueWriteBufferRect(queue, B, CL_FALSE, origin, origin, region2, region2[0] * sizeof(int), 0, region[0] * sizeof(int), 0, b, 0, NULL, e + 4);
	error(err, "could not enqueue rect buffer write from b to B");

	// create our kernel
	cl_kernel kern = clCreateKernel(prog, "matrix_multiplication", &err);
	error(err, "could not create kernel");
	err = clSetKernelArg(kern, 0, sizeof(cl_mem), &A);
	error(err, "could not set kernel argument 0");
	err = clSetKernelArg(kern, 1, sizeof(cl_mem), &B);
	error(err, "could not set kernel argument 1");
	err = clSetKernelArg(kern, 2, sizeof(cl_mem), &C);
	error(err, "could not set kernel argument 2");
	err = clSetKernelArg(kern, 3, sizeof(cl_mem), &M);
	error(err, "could not set kernel argument 3");
	
	size_t work_dim[2];
	work_dim[0] = m1;
	work_dim[1] = n2;

	size_t local_dim[2];
	local_dim[0] = 1;
	local_dim[1] = 1;

	err = clEnqueueNDRangeKernel(queue, kern, 2, origin, work_dim, local_dim, 5, e, e + 5);
	error(err, "could not enqueue kernel");

	size_t region3[3];
	region3[0] = n2;
	region3[1] = m1;
     	region3[2] = 1;	

	// read in our data
	err = clEnqueueReadBufferRect(queue, C, CL_TRUE, origin, origin, region3, region3[0] * sizeof(int), 0, region3[0] * sizeof(int), 0, c, 1, e + 5, NULL);
	error(err, "could not enqueue rect buffer read from C to c");

	// wait for the queue to complete
	err = clFlush(queue);
	error(err, "could not flush queue");
	err = clFinish(queue);
	error(err, "could not finish queue");

	// output our values
	
	printf("C: (%i x %i)\n", m1, n2);
	for(i = 0; i < m1; i++) {
		for(j = 0; j < n2; j++) {
			printf("%i ", c[i][j]);
		}
		printf("\n");
	}
	

	// clean up our resources
	err = clReleaseKernel(kern);
	error(err, "could not release kernel");
	err = clReleaseMemObject(A);
	error(err, "could not release buffer A");
	err = clReleaseMemObject(B);
	error(err, "could not release buffer B");
	err = clReleaseMemObject(C);
	error(err, "could not release buffer C");
	err = clReleaseMemObject(M);
	error(err, "could not release buffer M");
	err = clReleaseProgram(prog);
	error(err, "could not release program");
	err = clReleaseCommandQueue(queue);
	error(err, "could not release queue");
	err = clReleaseContext(context);
	error(err, "could not release context");

	free(a);
	free(b);
	free(c);	
	
}
