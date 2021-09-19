#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/raid/pq.h>

#define NDISKS		16	 

const char raid6_empty_zero_page[PAGE_SIZE] __attribute__((aligned(256)));
struct raid6_calls raid6_call;

char *dataptrs[NDISKS];
char data[NDISKS][PAGE_SIZE];
char recovi[PAGE_SIZE], recovj[PAGE_SIZE];

#ifdef MY_ABC_HERE
static void makedata(int start, int stop)
#else  
static void makedata(void)
#endif  
{
	int i, j;

#ifdef MY_ABC_HERE
	for (i = start; i <= stop; i++) {
#else  
	for (i = 0; i < NDISKS; i++) {
#endif  
		for (j = 0; j < PAGE_SIZE; j++)
			data[i][j] = rand();

		dataptrs[i] = data[i];
	}
}

static char disk_type(int d)
{
	switch (d) {
	case NDISKS-2:
		return 'P';
	case NDISKS-1:
		return 'Q';
	default:
		return 'D';
	}
}

static int test_disks(int i, int j)
{
	int erra, errb;

	memset(recovi, 0xf0, PAGE_SIZE);
	memset(recovj, 0xba, PAGE_SIZE);

	dataptrs[i] = recovi;
	dataptrs[j] = recovj;

	raid6_dual_recov(NDISKS, PAGE_SIZE, i, j, (void **)&dataptrs);

	erra = memcmp(data[i], recovi, PAGE_SIZE);
	errb = memcmp(data[j], recovj, PAGE_SIZE);

	if (i < NDISKS-2 && j == NDISKS-1) {
		 
		erra = errb = 0;
	} else {
		printf("algo=%-8s  faila=%3d(%c)  failb=%3d(%c)  %s\n",
		       raid6_call.name,
		       i, disk_type(i),
		       j, disk_type(j),
		       (!erra && !errb) ? "OK" :
		       !erra ? "ERRB" :
		       !errb ? "ERRA" : "ERRAB");
	}

	dataptrs[i] = data[i];
	dataptrs[j] = data[j];

	return erra || errb;
}

int main(int argc, char *argv[])
{
	const struct raid6_calls *const *algo;
	const struct raid6_recov_calls *const *ra;
#ifdef MY_ABC_HERE
	int i, j, p1, p2;
#else  
	int i, j;
#endif  
	int err = 0;

#ifdef MY_ABC_HERE
	makedata(0, NDISKS-1);
#else  
	makedata();
#endif  

	for (ra = raid6_recov_algos; *ra; ra++) {
		if ((*ra)->valid  && !(*ra)->valid())
			continue;

		raid6_2data_recov = (*ra)->data2;
		raid6_datap_recov = (*ra)->datap;

		printf("using recovery %s\n", (*ra)->name);

		for (algo = raid6_algos; *algo; algo++) {
#ifdef MY_ABC_HERE
			if ((*algo)->valid && !(*algo)->valid())
				continue;

			raid6_call = **algo;

			memset(data[NDISKS-2], 0xee, 2*PAGE_SIZE);

			raid6_call.gen_syndrome(NDISKS, PAGE_SIZE,
						(void **)&dataptrs);

			for (i = 0; i < NDISKS-1; i++)
				for (j = i+1; j < NDISKS; j++)
					err += test_disks(i, j);

			if (!raid6_call.xor_syndrome)
				continue;

			for (p1 = 0; p1 < NDISKS-2; p1++)
				for (p2 = p1; p2 < NDISKS-2; p2++) {

					raid6_call.xor_syndrome(NDISKS, p1, p2, PAGE_SIZE,
								(void **)&dataptrs);
					makedata(p1, p2);
					raid6_call.xor_syndrome(NDISKS, p1, p2, PAGE_SIZE,
                                                                (void **)&dataptrs);

					for (i = 0; i < NDISKS-1; i++)
						for (j = i+1; j < NDISKS; j++)
							err += test_disks(i, j);
				}

#else  
			if (!(*algo)->valid || (*algo)->valid()) {
				raid6_call = **algo;

				memset(data[NDISKS-2], 0xee, 2*PAGE_SIZE);

				raid6_call.gen_syndrome(NDISKS, PAGE_SIZE,
							(void **)&dataptrs);

				for (i = 0; i < NDISKS-1; i++)
					for (j = i+1; j < NDISKS; j++)
						err += test_disks(i, j);
			}
#endif  
		}
		printf("\n");
	}

	printf("\n");
	 
	raid6_select_algo();

	if (err)
		printf("\n*** ERRORS FOUND ***\n");

	return err;
}
