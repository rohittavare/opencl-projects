#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <CL/cl.h>

int m1 = 10;
int n1 = 10;
int m2 = n1;
int n2 = 10;

int main(int argc, char ** argv) {
	
	// create out data
	int ** a = (int **)malloc(m1 * sizeof(int *));
	int ** b = (int **)malloc(m2 * sizeof(int *));
	int ** c = (int **)malloc(m1 * sizeof(int *));

	int i;
	int j;
	for(i = 0; i < m1; i++) {
		a[i] = (int *)malloc(n1 * sizeof(int));
		for(j = 0; j < n1; j++) {
			a[i][j] = (i + j) % n1; 
		}
	}
	for(i = 0; i < m2; j++) {
		b[i] = (int *)malloc(n2 * sizeof(int));
		for(j = 0; j < n2; j++) {
			b[i][j] = (i + j) % m2;
		}
	}
	for(i = 0; i < m1; i++) {
		c[i] = (int *)malloc(n2 * sizeof(int));
	}

	// read in our kernel file
	char * source = (char *) malloc(300 * sizeof(char));
	int src_file = open("kernel.cl", O_RDONLY);
	int src_len = read(src_file, source, 300);
	
	cl_int err;
	
	// query our platform and its device
	cl_platform_id platform;
	cl_device_id device;
	
	err = clGetPlatformIDs( 1, &platform, NULL);
	err = clGetDeviceIDs(platform, CL_DEVICE_DEFAULT, 1, &device, NULL);

	// create a context
	cl_context context = clCreateContext(NULL, 1, device, NULL, NULL, &err);

	// create our queue
	cl_queue_properties * props = (cl_queue_properties *)malloc(3 * sizeof(cl_queue_properties));
	props[0] = CL_QUEUE_PROPERTIES;
	props[1] = CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
	props[2] = 0;

	cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, props, &err);

	// create the program object
	cl_program prog = clCreateProgramWithSource(context, 1, (const char **) &source, &source_len, &err);
	err = clBuildProgram(prog, 1, &device, NULL, NULL, NULL);

	// create our buffers
	cl_mem A = clCreateBuffer(context, CL_MEM_READ_ONLY, m1 * n1 * sizeof(int), NULL, &err);
	cl_mem B = clCreateBuffer(context, CL_MEM_READ_ONLY, m2 * n2 * sizeof(int), NULL, &err);
	cl_mem C = clCreateBuffer(context, CL_MEM_WRITE_ONLY, m1 * n2 * sizeof(int), NULL, &err);


}
