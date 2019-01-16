#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <CL/cl.h>

size_t arr_size = 10;

int main() {

	// read in the kernel source
	int file = open("kernel.cl", O_RDONLY);
	char * source = (char *) malloc(300 * sizeof(char));
	size_t source_len = read(file, source, 300);

	// create our arrays
	int * a = (int *) malloc(arr_size * sizeof(int));
	int * b = (int *) malloc(arr_size * sizeof(int));
	int * c = (int *) malloc(arr_size * sizeof(int));
	int * d = (int *) malloc(arr_size * sizeof(int));
	int * e = (int *) malloc(arr_size * sizeof(int));

	size_t i;

	for(i = 0; i < arr_size; i++) {
		a[i] = i;
		c[i] = i;
		b[i] = arr_size - i;
		d[i] = arr_size - i;
	}

	cl_int err; // for error detection

	// query our devices
	cl_platform_id platform;
	cl_device_id device;

	err = clGetPlatformIDs(1, &platform, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not get platform IDs\n"); exit(1); }
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not get device IDs\n"); exit(1); }

	// create our context
	cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create context\n"); exit(1); }

	// crete our unordered command queue
	cl_queue_properties * properties = malloc(3 * sizeof(cl_queue_properties));
	properties[0] = CL_QUEUE_PROPERTIES;
	properties[1] = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
	properties[2] = 0;
	cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, properties, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create unordered command queue\n"); exit(1); }

	// create our program
	cl_program program = clCreateProgramWithSource(context, 1,(const char **) &source, &source_len, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create program using source file\n"); exit(1); }
	err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	if(err != CL_SUCCESS) {	fprintf(stderr, "error: could not build program\n"); exit(1); }

	// create our buffers
	cl_mem A = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer A\n"); exit(1); }
	cl_mem B = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer B\n"); exit(1); }
	cl_mem C = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer C\n"); exit(1); }
	cl_mem D = clCreateBuffer(context, CL_MEM_READ_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer D\n"); exit(1); }
	cl_mem E = clCreateBuffer(context, CL_MEM_READ_WRITE, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer E\n"); exit(1); }
	cl_mem F = clCreateBuffer(context, CL_MEM_READ_WRITE, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer F\n"); exit(1); }
	cl_mem G = clCreateBuffer(context, CL_MEM_WRITE_ONLY, arr_size * sizeof(int), NULL, &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create buffer G\n"); exit(1); }

	cl_event * events = (cl_event *) malloc(8 * sizeof(cl_event));

	// write to our buffers
	err = clEnqueueWriteBuffer(queue, A, CL_FALSE, 0, arr_size * sizeof(int), a, 0, NULL, events);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not write to A from a\n"); exit(1); }
	err = clEnqueueWriteBuffer(queue, B, CL_FALSE, 0, arr_size * sizeof(int), b, 0, NULL, events + 1);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not write to B from b\n"); exit(1); }
	err = clEnqueueWriteBuffer(queue, C, CL_FALSE, 0, arr_size * sizeof(int), c, 0, NULL, events + 2);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not write to C from c\n"); exit(1); }
	err = clEnqueueWriteBuffer(queue, D, CL_FALSE, 0, arr_size * sizeof(int), d, 0, NULL, events + 3);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not write to D from d\n"); exit(1); }

	// create our kernel
	cl_kernel kernel_add = clCreateKernel(program, "vector_addition", &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create addition kernel\n"); exit(1); }
	cl_kernel kernel_mult = clCreateKernel(program, "vector_multiplication", &err);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not create multiplication kernel\n"); exit(1); }

	// set kernel arguments and enqueue
	err = clSetKernelArg(kernel_mult, 0, sizeof(cl_mem), &A);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplication kernel argument 0 (line 96)\n");
	err = clSetKernelArg(kernel_mult, 1, sizeof(cl_mem), &B);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplication kernel argument 1 (line 98)\n");
	err = clSetKernelArg(kernel_mult, 2, sizeof(cl_mem), &E);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplication kernel argument 2 (line 100)\n");

	cl_uint dim = 1;
	size_t offset = 0;
	size_t local_size = 16;

	err = clEnqueueNDRangeKernel(queue, kernel_mult, dim, &offset, &arr_size, &local_size, 2, events, events + 4);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not enqueue multiplication kernel\n"); exit(1); }

	err = clSetKernelArg(kernel_mult, 0, sizeof(cl_mem), &C);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplcation kernel argument 0 (line 108)\n");
	err = clSetKernelArg(kernel_mult, 1, sizeof(cl_mem), &D);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplication kernel argument 1 (line 110)\n");
	err = clSetKernelArg(kernel_mult, 2, sizeof(cl_mem), &F);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set multiplication kernel argument 2 (line 112)\n");

	err = clEnqueueNDRangeKernel(queue, kernel_mult, dim, &offset, &arr_size, &local_size, 2, events + 2, events + 5);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not enqueue multiplication kernel\n"); exit(1); }

	err = clSetKernelArg(kernel_add, 0, sizeof(cl_mem), &E);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set addition kernel argument 0\n");
	err = clSetKernelArg(kernel_add, 1, sizeof(cl_mem), &F);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set addition kernel argument 1\n");
	err = clSetKernelArg(kernel_add, 2, sizeof(cl_mem), &G);
	if(err != CL_SUCCESS) fprintf(stderr, "error: could not set addition kernel argument 2\n");

	err = clEnqueueNDRangeKernel(queue, kernel_add, dim, &offset, &arr_size, &local_size, 2, events + 4, events + 6);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not enqueue addition kernel\n"); exit(1); }

	// read in the result
	err = clEnqueueReadBuffer(queue, G, CL_TRUE, 0, arr_size * sizeof(int), e, 1, events + 6, NULL);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not read buffer G into e\n"); exit(1); }

	err = clFlush(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not flush queue\n"); exit(1); }
	err = clFinish(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not finish queue\n"); exit(1); }

	// output our results
	printf("e = a*b + c*d\n");
	printf("a: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", a[i]);
	}
	printf("\n");
	printf("b: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", b[i]);
	}
	printf("\n");
	printf("c: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", c[i]);
	}
	printf("\n");
	printf("d: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", d[i]);
	}
	printf("\n");
	printf("e: ");
	for(i = 0; i < arr_size; i++) {
		printf("%i ", e[i]);
	}
	printf("\n");

	// free our cl resources
	err = clReleaseKernel(kernel_add);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release addition kernel\n"); exit(1); }
	err = clReleaseKernel(kernel_mult);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release multiplication kernel\n"); exit(1); }
	err = clReleaseMemObject(A);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object A\n"); exit(1); }
	err = clReleaseMemObject(B);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object B\n"); exit(1); }
	err = clReleaseMemObject(C);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object C\n"); exit(1); }
	err = clReleaseMemObject(D);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object D\n"); exit(1); }
	err = clReleaseMemObject(E);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object E\n"); exit(1); }
	err = clReleaseMemObject(F);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object F\n"); exit(1); }
	err = clReleaseMemObject(G);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release memory object G\n"); exit(1); }
	err = clReleaseProgram(program);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release program\n"); exit(1); }
	err = clReleaseCommandQueue(queue);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release queue\n"); exit(1); }
	err = clReleaseContext(context);
	if(err != CL_SUCCESS) { fprintf(stderr, "error: could not release context\n"); exit(1); }

	// free memory
	free(a);
	free(b);
	free(c);
	free(d);
	free(e);
	free(source);
	free(properties);
	free(events);

}
