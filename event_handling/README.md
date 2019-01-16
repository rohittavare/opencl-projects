### Event Handling

This demo is based on exercise 2 (page 56) from [these slides](https://www.fz-juelich.de/SharedDocs/Downloads/IAS/JSC/EN/slides/opencl/opencl-03-basics.pdf?__blob=publicationFile) from Forschungszentrum Jülich.

The demo calculates vector `e` using the following formula the ith element of the result vector: `e[i] = a[i] * b[i] + c[i] * d[i]`

Event handling is achieved by supplying `CL_QUEUE_PROPERTIES` followed by `CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE` in the option field in `clCreateCommandQueueWithProperties()`. The `event_wait_list` field describes a list of events that the system will wait to complete before executing the enqueued task. Each enqueue call has an `event` field which can be using in `event_wait_list` to provide some form of ordering.

run `make build` or `make` to build the program. `make clean` to remove the executable.
