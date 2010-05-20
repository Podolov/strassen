/* 
Compile options
/usr/lib/openmpi/1.4-gcc/bin/mpicc -fopenmp
/usr/lib/openmpi/1.4-gcc/bin/mpirun --mca btl tcp,self,sm --hostfile my_hostfile -np 7 a.out
*/

#include <omp.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
//#include <sys/time.h>
//#include <sys/types.h>

//matrix dimensions
#define DIM_N 2048
#define threads 4
#define threshold 64
int chunk=1;

//other stuff
double sum, snorm;
double *d;

//matrices
double *A, *B, *C;

//Prototypes
void strassenMultMatrix(double*,double*,double*,int);
void normalMultMatrix(double*, double*, double*, int);
void catMatrix(double*, double*,double*,double*,double*, int);
void splitMatrix(double*, double*, double*,double*,double*, int);
void subMatrices(double*, double*, double*, int);
void addMatrices(double*, double*, double*, int);

//MAIN
int main (int argc, char *argv[]){
  int nproc, myid, newsize;
  int i, j, k, a, b, c, step;
  int prev, next, start;
  double stime, ntime;
  double *a11, *a22, *a12, *a21;
  double *b11, *b22, *b12, *b21;
  double *m1, *m2, *m3, *m4, *m5, *m6, *m7;
  double *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *t9, *t10;
  //Start the MPI session
  MPI_Init(&argc,&argv);

  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  
  newsize = (int)DIM_N/2;
  
  A = (double*) malloc(sizeof(double)*DIM_N*DIM_N);

  B = (double*) malloc(sizeof(double)*DIM_N*DIM_N);

  C = (double*) malloc(sizeof(double)*DIM_N*DIM_N);

  if (myid == 0){
    d = (double*) malloc(sizeof(double)*DIM_N);
    sum = 0.0; snorm = 0.0;
    for (i = 0; i < DIM_N; i++){
      d[i] = drand48();
      snorm += d[i]*d[i];
    }
    for (i = 0; i < DIM_N; i++){
      d[i] = d[i]/sqrt(snorm);
      sum += d[i]*d[i];
    }

    for (i = 0; i < DIM_N; i++)
      for (j = 0; j < DIM_N; j++)
      A[i*DIM_N + j] =  -2*d[i]*d[j];

    for (i = 0; i < DIM_N; i++)
      A[i*DIM_N + i] = 1.0 + A[i*DIM_N + i];

    for (j = 0; j < DIM_N; j++)
      for (k = 0; k < DIM_N; k++)
      B[j*DIM_N + k] = -2*d[j]*d[k];

    for (j = 0; j < DIM_N; j++)
      B[j*DIM_N + j] = 1.0 + B[j*DIM_N + j];
  
    printf("Num procs = %d\n",nproc);
  }
  //clear C matrix 
  if (myid==0){
	  for (a = 0; a<DIM_N; a++)
		  for (b = 0; b<DIM_N; b++)
			  C[a][b] = 0;
	  printf("Created the Matrices A and B\n");
	}
	
	//create the initial distributions
	if (myid == 0){
      //Allocate memory....this could get expensive pretty quickly
    a11 = (double*) malloc(sizeof(double)*newsize*newsize);
    a12 = (double*) malloc(sizeof(double)*newsize*newsize);
    a21 = (double*) malloc(sizeof(double)*newsize*newsize);
    a22 = (double*) malloc(sizeof(double)*newsize*newsize);
    b11 = (double*) malloc(sizeof(double)*newsize*newsize);
    b12 = (double*) malloc(sizeof(double)*newsize*newsize);
    b21 = (double*) malloc(sizeof(double)*newsize*newsize);
    b22 = (double*) malloc(sizeof(double)*newsize*newsize);
    m1 = (double*) malloc(sizeof(double)*newsize*newsize);
    m2 = (double*) malloc(sizeof(double)*newsize*newsize);
    m3 = (double*) malloc(sizeof(double)*newsize*newsize);
    m4 = (double*) malloc(sizeof(double)*newsize*newsize);
    m5 = (double*) malloc(sizeof(double)*newsize*newsize);
    m6 = (double*) malloc(sizeof(double)*newsize*newsize);
    m7 = (double*) malloc(sizeof(double)*newsize*newsize);
    t1 = (double*) malloc(sizeof(double)*newsize*newsize);
    t2 = (double*) malloc(sizeof(double)*newsize*newsize);
    t3 = (double*) malloc(sizeof(double)*newsize*newsize);
    t4 = (double*) malloc(sizeof(double)*newsize*newsize);
    t5 = (double*) malloc(sizeof(double)*newsize*newsize);
    t6 = (double*) malloc(sizeof(double)*newsize*newsize);
    t7 = (double*) malloc(sizeof(double)*newsize*newsize);
    t8 = (double*) malloc(sizeof(double)*newsize*newsize);
    t9 = (double*) malloc(sizeof(double)*newsize*newsize);
    t10 = (double*) malloc(sizeof(double)*newsize*newsize);
  
    splitMatrix(A,a11,a12,a21,a22,newsize);
    splitMatrix(B,b11,b12,b21,b22,newsize);
    
    addMatrices(a11,a22,t1,newsize);
    addMatrices(a21,a22,t2,newsize);
    addMatrices(a11,a12,t3,newsize);
    subMatrices(a21,a11,t4,newsize);
    subMatrices(a12,a22,t5,newsize);
    addMatrices(b11,b22,t6,newsize);
    subMatrices(b12,b22,t7,newsize);
    subMatrices(b21,b11,t8,newsize);
    addMatrices(b11,b12,t9,newsize);
    addMatrices(b21,b22,t10,newsize);
    printf("Sending....\n\n");
    //for (i=0; i<newsize; i++){
    MPI_Send(t2,newsize*newsize,MPI_DOUBLE,1,0,MPI_COMM_WORLD);
    MPI_Send(b11,newsize*newsize,MPI_DOUBLE,1,1,MPI_COMM_WORLD);
    MPI_Send(a11,newsize*newsize,MPI_DOUBLE,2,0,MPI_COMM_WORLD);
    MPI_Send(t7,newsize*newsize,MPI_DOUBLE,2,1,MPI_COMM_WORLD);
    MPI_Send(a22,newsize*newsize,MPI_DOUBLE,3,0,MPI_COMM_WORLD);
    MPI_Send(t8,newsize*newsize,MPI_DOUBLE,3,1,MPI_COMM_WORLD);
    MPI_Send(t3,newsize*newsize,MPI_DOUBLE,4,0,MPI_COMM_WORLD);
    MPI_Send(b22,newsize*newsize,MPI_DOUBLE,4,1,MPI_COMM_WORLD);
    MPI_Send(t4,newsize*newsize,MPI_DOUBLE,5,0,MPI_COMM_WORLD);
    MPI_Send(t9,newsize*newsize,MPI_DOUBLE,5,1,MPI_COMM_WORLD);
    MPI_Send(t5,newsize*newsize,MPI_DOUBLE,6,0,MPI_COMM_WORLD);
    MPI_Send(t10,newsize*newsize,MPI_DOUBLE,6,1,MPI_COMM_WORLD);
    //}

    //Start TimerMPI_COMM_WORLD
    stime = MPI_Wtime();
    
    strassenMultMatrix(t1,t6,m1,newsize);
    
    printf("Mult done, receiving.... \n");
    //for (i=0; i<newsize; i++){
    MPI_Recv(m2,newsize*newsize,MPI_DOUBLE,1,1,MPI_COMM_WORLD,NULL);
    MPI_Recv(m3,newsize*newsize,MPI_DOUBLE,2,2,MPI_COMM_WORLD,NULL);
    MPI_Recv(m4,newsize*newsize,MPI_DOUBLE,3,3,MPI_COMM_WORLD,NULL);
    MPI_Recv(m5,newsize*newsize,MPI_DOUBLE,4,4,MPI_COMM_WORLD,NULL);
    MPI_Recv(m6,newsize*newsize,MPI_DOUBLE,5,5,MPI_COMM_WORLD,NULL);
    MPI_Recv(m7,newsize*newsize,MPI_DOUBLE,6,6,MPI_COMM_WORLD,NULL);
    //}
    printf("Done, receiving\n");
    addMatrices(m1,m4,a11,newsize);
    subMatrices(m5,m7,a12,newsize);
    addMatrices(m3,m1,a21,newsize);
    subMatrices(m2,m6,a22,newsize);
    subMatrices(a11,a12,b11,newsize);
    addMatrices(m3,m5,b12,newsize);
    addMatrices(m2,m4,b21,newsize);
    subMatrices(a21,a22,b22,newsize);
    
    catMatrix(C,b11,b12,b21,b22,newsize);

    //Stop Timer
    ntime =  MPI_Wtime();
    
    free(a11);free(a12);free(a21);free(a22);
    free(b11);free(b12);free(b21);free(b22);
    free(t1);free(t2);free(t3);free(t4);free(t5);
    free(t6);free(t7);free(t8);free(t9);free(t10);
    free(m1);free(m2);free(m3);free(m4);free(m5);free(m6);free(m7);

	} else if (myid>0 && myid<7){
	  t1 = (double*) malloc(sizeof(double)*newsize*newsize);
    t2 = (double*) malloc(sizeof(double)*newsize*newsize);
    t3 = (double*) malloc(sizeof(double)*newsize*newsize);

    //printf("No %d Receiving....\n\n",myid);
    //for (i=0; i < newsize; i++){
    MPI_Recv(t1,newsize*newsize,MPI_DOUBLE,0,0,MPI_COMM_WORLD,NULL);
    MPI_Recv(t2,newsize*newsize,MPI_DOUBLE,0,1,MPI_COMM_WORLD,NULL);
    //}
    //printf("done receiving %d\n",myid);
    strassenMultMatrix(t1,t2,t3,newsize);
    //printf("Mult done, sending back %d\n",myid);
    //for (i=0; i < newsize; i++){
    MPI_Send(t3,newsize*newsize,MPI_DOUBLE,0,myid,MPI_COMM_WORLD);
    //}
    //printf("done sending back %d\n",myid);
    free(t1);free(t2);free(t3);
	}

  /*if(myid==0){
    for (i=0; i<DIM_N; i++){
      for (j=0; j<DIM_N; j++)
        printf("%lf ",B[i][j]);
      printf("\n");
    }
  }*/
  if(myid == 0)
    printf("N = %d \tTime taken = %f \n", DIM_N, ntime-stime);
          
  //End MPI session
  MPI_Finalize();
  

}
/*****************************************************************************
 * Note that all following functions only deal with square matrices          *
 * where N is divisible by 2.                                                *
 *****************************************************************************/
void addMatrices(double *x, double *y, double *z, int size){
//performs a matrix addition operation, z=x+y
	int i;
	#pragma omp parallel shared(x,y,z,size,chunk) private(i,j) num_threads(threads) 
	{
     #pragma omp for schedule(dynamic,chunk) nowait
	      for (i = 0; i < size*size; i++)
		      z[i] = x[i] + y[i];  
	}
}

void subMatrices(double *x, double *y, double *z, int size){
//performs a matrix subtraction operation, z=x-y
	int i;
	#pragma omp parallel shared(x,y,z,size,chunk) private(i,j) num_threads(threads)
	{
     #pragma omp for schedule(dynamic,chunk) nowait
	      for (i = 0; i < size*size; i++)
		      z[i] = x[i] - y[i];
	}
}

void splitMatrix(double *a, double *a11,double *a12,double *a21,double *a22, int size){
//takes a matrix a adn splits it into its 4 quadrants.
	int i,j,x,y;
	int newsize = (int)size/2;
	x=0; y=0;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,newsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < newsize+x; i++)
	      for (j = y; j < newsize+y; j++)
		      a11[(i-x)*newsize + j-y] = a[i*newsize + j];
	}
	x=newsize; y=0;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,newsize,chunk) private(i,j) num_threads(threads)
	{
     #pragma omp for schedule(dynamic,chunk) nowait
	      for (i = x; i < newsize+x; i++)
		      for (j = y; j < newsize+y; j++)
			      a12[(i-x)*newsize + j-y] = a[i*newsize + j];
	}
	x=0; y=newsize;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,newsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < newsize+x; i++)
        for (j = y; j < newsize+y; j++)
          a21[(i-x)*newsize + j-y] = a[i*newsize + j];
	}
	x=newsize; y=newsize;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,newsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < newsize+x; i++)
        for (j = y; j < newsize+y; j++)
          a22[(i-x)*newsize + j-y] = a[i*newsize + j];
	}
}

void catMatrix(double *a, double *a11,double *a12,double *a21,double *a22, int size){
//does the inverse of the splitMatrix function
	int i,j,x,y;
	int oldsize = (int)size/2;
  x=0; y=0;
  #pragma omp parallel shared(a,a11,a12,a21,a22,x,y,oldsize,chunk) private(i,j) num_threads(threads)
  {
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < oldsize+x; i++)
        for (j = y; j < oldsize+y; j++)
          a[i*oldsize + j] = a11[(i-x)*oldsize + j-y];
	}
	x=oldsize; y=0;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,oldsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < oldsize+x; i++)
        for (j = y; j < oldsize+y; j++)
          a[i*oldsize + j] = a12[(i-x)*oldsize + j-y];
	}
	x=0; y=oldsize;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,oldsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < oldsize+x; i++)
        for (j = y; j < oldsize+y; j++)
          a[i*oldsize + j] = a21[(i-x)*oldsize + j-y];
	}
	x=oldsize; y=oldsize;
	#pragma omp parallel shared(a,a11,a12,a21,a22,x,y,oldsize,chunk) private(i,j) num_threads(threads)
	{
    #pragma omp for schedule(dynamic,chunk) nowait
      for (i = x; i < oldsize+x; i++)
        for (j = y; j < oldsize+y; j++)
          a[i*oldsize + j] = a22[(i-x)*oldsize + j-y];
	}
}

void normalMultMatrix(double *x, double *y, double *z, int size){
//multiplys two matrices: z=x*y
	int i,j,k;
	
	#pragma omp parallel shared(A,B,C,chunk) private(i,j,k) num_threads(threads)
	{
	  //multiplication process
    #pragma omp for schedule(dynamic) nowait
	    for (j = 0; j < size; j++){
		    for (i = 0; i < size; i++){
		      z[i*size + j] = 0.0;
			    for (k = 0; k < size; k++)
				    z[i*size + j] += x[i*size + k] * y[k*size + j];
				}
      }
	}
}

void strassenMultMatrix(double *a,double *b,double *c,int size){
//Performs a Strassen matrix multiply operation
//This does miracles, and is recursive
//To perform a miracle, it first performs a miracle
  double *a11, *a22, *a12, *a21;
  double *b11, *b22, *b12, *b21;
  double *m1, *m2, *m3, *m4, *m5, *m6, *m7; 
  double *t1, *t2, *t3, *t4, *t5, *t6, *t7, *t8, *t9, *t10;
  int newsize = (int)size/2;
  int i;
  if (size > threshold) {
    //Allocate memory....this could get expensive pretty quickly
    a11 = (double**) malloc(sizeof(double)*newsize*newsize);
    a12 = (double**) malloc(sizeof(double)*newsize*newsize);
    a21 = (double**) malloc(sizeof(double)*newsize*newsize);
    a22 = (double**) malloc(sizeof(double)*newsize*newsize);
    b11 = (double**) malloc(sizeof(double)*newsize*newsize);
    b12 = (double**) malloc(sizeof(double)*newsize*newsize);
    b21 = (double**) malloc(sizeof(double)*newsize*newsize);
    b22 = (double**) malloc(sizeof(double)*newsize*newsize);
    m1 = (double**) malloc(sizeof(double)*newsize*newsize);
    m2 = (double**) malloc(sizeof(double)*newsize*newsize);
    m3 = (double**) malloc(sizeof(double)*newsize*newsize);
    m4 = (double**) malloc(sizeof(double)*newsize*newsize);
    m5 = (double**) malloc(sizeof(double)*newsize*newsize);
    m6 = (double**) malloc(sizeof(double)*newsize*newsize);
    m7 = (double**) malloc(sizeof(double)*newsize*newsize);
    t1 = (double**) malloc(sizeof(double)*newsize*newsize);
    t2 = (double**) malloc(sizeof(double)*newsize*newsize);
    t3 = (double**) malloc(sizeof(double)*newsize*newsize);
    t4 = (double**) malloc(sizeof(double)*newsize*newsize);
    t5 = (double**) malloc(sizeof(double)*newsize*newsize);
    t6 = (double**) malloc(sizeof(double)*newsize*newsize);
    t7 = (double**) malloc(sizeof(double)*newsize*newsize);
    t8 = (double**) malloc(sizeof(double)*newsize*newsize);
    t9 = (double**) malloc(sizeof(double)*newsize*newsize);
    t10 = (double**) malloc(sizeof(double)*newsize*newsize);

    splitMatrix(a,a11,a12,a21,a22,size);
    splitMatrix(b,b11,b12,b21,b22,size);
    
    addMatrices(a11,a22,t1,newsize);
    addMatrices(a21,a22,t2,newsize);
    addMatrices(a11,a12,t3,newsize);
    subMatrices(a21,a11,t4,newsize);
    subMatrices(a12,a22,t5,newsize);
    addMatrices(b11,b22,t6,newsize);
    subMatrices(b12,b22,t7,newsize);
    subMatrices(b21,b11,t8,newsize);
    addMatrices(b11,b12,t9,newsize);
    addMatrices(b21,b22,t10,newsize);
    
    strassenMultMatrix(t1,t6,m1,newsize);
    strassenMultMatrix(t2,b11,m2,newsize);
    strassenMultMatrix(a11,t7,m3,newsize);
    strassenMultMatrix(a22,t8,m4,newsize);
    strassenMultMatrix(t3,b22,m5,newsize);
    strassenMultMatrix(t4,t9,m6,newsize);
    strassenMultMatrix(t5,t10,m7,newsize);
    
    addMatrices(m1,m4,a11,newsize);
    subMatrices(m5,m7,a12,newsize);
    addMatrices(m3,m1,a21,newsize);
    subMatrices(m2,m6,a22,newsize);
    subMatrices(a11,a12,b11,newsize);
    addMatrices(m3,m5,b12,newsize);
    addMatrices(m2,m4,b21,newsize);
    subMatrices(a21,a22,b22,newsize);
    
    catMatrix(c,b11,b12,b21,b22,size);
    free(a11);free(a12);free(a21);free(a22);
    free(b11);free(b12);free(b21);free(b22);
    free(t1);free(t2);free(t3);free(t4);free(t5);free(t6);free(t7);free(t8);free(t9);free(t10);
    free(m1);free(m2);free(m3);free(m4);free(m5);free(m6);free(m7);
  }
  else {
    normalMultMatrix(a,b,c,size);
    //c[0][0]=a[0][0]*b[0][0];
  }
}