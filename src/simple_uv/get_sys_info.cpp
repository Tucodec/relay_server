#ifdef WIN32
double get_cpus_usage(bool&enable_test) {
	double percentage = 0;
	return percentage;
}
#else
/*
*         Author:  SITKI BURAK CALIM (https://github.com/sbcalim), sburakcalim@gmail.com
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define PROCSTAT "/proc/stat"

unsigned long long*read_cpu() {
	FILE *fp;
	unsigned long long *array;
	unsigned long long ignore[6];
	fp = fopen(PROCSTAT, "r");
	array = (unsigned long long *)malloc(4 * sizeof(unsigned long long));
	fscanf(fp, "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu ",
		&array[0], &array[1], &array[2], &array[3],
		&ignore[0], &ignore[1], &ignore[2], &ignore[3], &ignore[4], &ignore[5]);
	fclose(fp);
	return array;
}

/*
* ===  FUNCTION  ======================================================================
*         Name:  get_cpu_percentage
*  Description:  Returns a double value of CPU usage percentage from given array[4] = {USER_PROC, NICE_PROC, SYSTEM_PROC, IDLE_PROC}
* =====================================================================================
*/
double get_cpu_percentage(unsigned long long *a1, unsigned long long *a2) {
	return (double)(((double)((a1[0] - a2[0]) + (a1[1] - a2[1]) + (a1[2] - a2[2])) /
		(double)((a1[0] - a2[0]) + (a1[1] - a2[1]) + (a1[2] - a2[2]) + (a1[3] - a2[3]))) * 100);
}

double get_cpus_usage(bool&enable_test) {
	double percentage = 0;
	unsigned long long *p_cpu_array = read_cpu();
	while (enable_test) {
		sleep(1);
		unsigned long long *n_cpu_array = read_cpu();
		double temp = get_cpu_percentage(n_cpu_array, p_cpu_array);
		free(p_cpu_array);
		p_cpu_array = n_cpu_array;
		n_cpu_array = NULL;
		percentage = percentage>temp ? percentage : temp;
	}
	free(p_cpu_array);
	return percentage;
}

#endif