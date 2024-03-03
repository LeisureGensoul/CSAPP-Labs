/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int t0, t1, t2, t3, t4, t5, t6, t7;
	
	if (M == 32){
		for (int n=0; n < N; n += 8) {
			for (int m=0; m < M; m += 8) {
				for (int i=n; i < n+8; ++i) {
					// 对角矩阵的情况
					if (m==n) {
						// 一次性取出A的一行
						t0 = A[i][m];
						t1 = A[i][m+1];
						t2 = A[i][m+2];
						t3 = A[i][m+3];
						t4 = A[i][m+4];
						t5 = A[i][m+5];
						t6 = A[i][m+6];
						t7 = A[i][m+7];

						B[m][i] = t0;
						B[m+1][i] = t1;
						B[m+2][i] = t2;
						B[m+3][i] = t3;
						B[m+4][i] = t4;
						B[m+5][i] = t5;
						B[m+6][i] = t6;
						B[m+7][i] = t7;
					} else {    // 非对角矩阵的情况
						for (int j=m; j < m+8; ++j) {
							B[j][i] = A[i][j];
						}
					}
				}
			}
		}
	} else if (M == 64) {
		// 还是8x8分块，这个没有变
		for (int i = 0; i < N; i += 8) { 
			for (int j = 0; j < M; j += 8) {
				for (int k = i; k < i + 4; ++k) { //把a的前四行赋给b的前四行
					t0 = A[k][j + 0];
					t1 = A[k][j + 1];
					t2 = A[k][j + 2];
					t3 = A[k][j + 3];
					t4 = A[k][j + 4];
					t5 = A[k][j + 5];
					t6 = A[k][j + 6];
					t7 = A[k][j + 7];

					B[j + 0][k] = t0;
					B[j + 1][k] = t1;
					B[j + 2][k] = t2;
					B[j + 3][k] = t3;
					B[j + 0][k + 4] = t4;
					B[j + 1][k + 4] = t5;
					B[j + 2][k + 4] = t6;
					B[j + 3][k + 4] = t7;
				}

				for (int k = j; k < j + 4; ++k) {
					//获取B的右上部分
					t0 = B[k][i + 4];
					t1 = B[k][i + 5];
					t2 = B[k][i + 6];
					t3 = B[k][i + 7];

					//获取A的左下部分，这里一次性miss4次
					t4 = A[i + 4][k];
					t5 = A[i + 5][k];
					t6 = A[i + 6][k];
					t7 = A[i + 7][k];

					//将A的左下写入B的右上
					B[k][i + 4] = t4;
					B[k][i + 5] = t5;
					B[k][i + 6] = t6;
					B[k][i + 7] = t7;

					//将之前B右上部分写入左下部分， 这里每次迭代miss一次，总共4次
					B[k + 4][i] = t0;
					B[k + 4][i + 1] = t1;
					B[k + 4][i + 2] = t2;
					B[k + 4][i + 3] = t3;
				}

				for (int k = i + 4; k < i + 8; ++k) {
					//此时A、B的右下都在缓存中
					t0 = A[k][j + 4];
					t1 = A[k][j + 5];
					t2 = A[k][j + 6];
					t3 = A[k][j + 7];

					B[j + 4][k] = t0;
					B[j + 5][k] = t1;
					B[j + 6][k] = t2;
					B[j + 7][k] = t3;
				}
			}
		}
	} else if (M == 61) {
		int size=17;
		int temp;
		for (int n=0; n<N; n+=size) {
			for (int m=0; m<M; m+=size) {
				for (int i=n; i<n+size&&i<N; i++) {
					for (int j=m; j<m+size&&j<M; j++) {
						temp = A[i][j];
						B[j][i]=temp;
					}
				}
			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

//分块成8*8的矩阵版本
char trans_32_desc[] = "8*8 matrix version";
void transpose_32(int M, int N, int A[N][M], int B[M][N]) {
    int temp;
    for (int n=0; n < N; n += 8) {
        for (int m=0; m < M; m += 8) {
            for (int i=n; i < n+8; ++i) {
                for (int j=m; j < m+8; ++j) {
                    temp = A[i][j];
                    B[j][i] = temp;
                }
            }
        }
    }
}

//分块成8*8的矩阵且带有对角线优化的版本
char trans_submit_32_desc[] = "8*8 matrix with diagonal version";
void transpose_submit_32(int M, int N, int A[N][M], int B[M][N]) {
    int temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    for (int n=0; n < N; n += 8) {
        for (int m=0; m < M; m += 8) {
            for (int i=n; i < n+8; ++i) {
                // 对角矩阵的情况
                if (m==n) {
                    // 一次性取出A的一行
                    temp0 = A[i][m];
                    temp1 = A[i][m+1];
                    temp2 = A[i][m+2];
                    temp3 = A[i][m+3];
                    temp4 = A[i][m+4];
                    temp5 = A[i][m+5];
                    temp6 = A[i][m+6];
                    temp7 = A[i][m+7];

                    B[m][i] = temp0;
                    B[m+1][i] = temp1;
                    B[m+2][i] = temp2;
                    B[m+3][i] = temp3;
                    B[m+4][i] = temp4;
                    B[m+5][i] = temp5;
                    B[m+6][i] = temp6;
                    B[m+7][i] = temp7;
                } else {    // 非对角矩阵的情况
                    for (int j=m; j < m+8; ++j) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

// 采用4x4子矩阵规模分块转置
char trans_64_desc[] = "4*4 matrix version";
void transpose_64(int M, int N, int A[N][M], int B[M][N]) {
    int temp0, temp1, temp2, temp3;
    for (int n=0; n < N; n += 4) {
        for (int m=0; m < M; m += 4) {
            for (int i=n; i < n+4; ++i) {
                // 对角矩阵的情况
                if (m==n) {
                    // 一次性取出A的一行
                    temp0 = A[i][m];
                    temp1 = A[i][m+1];
                    temp2 = A[i][m+2];
                    temp3 = A[i][m+3];

                    B[m][i] = temp0;
                    B[m+1][i] = temp1;
                    B[m+2][i] = temp2;
                    B[m+3][i] = temp3;
                } else {    // 非对角矩阵的情况
                    for (int j=m; j < m+4; ++j) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
}

//采用8*8分块，但转置的时候又按照4个4*4矩阵转置，常规移动顺序
char trans_test_64_desc[] = "8*8 matrix but trans with 4 4*4 son matrix version，normal order";
void transpose_test64(int M, int N, int A[N][M], int B[M][N])
{
    int t0, t1, t2, t3, t4, t5, t6, t7;
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
			//每个8x8分块A产生12次miss，B产生12次miss，总共是24次miss
            for (int k = i; k < i + 4; ++k) { //把a的前四行赋给b的前四行，循环四次，A产生4次miss，B产生4次miss
                t0 = A[k][j + 0];
                t1 = A[k][j + 1];
                t2 = A[k][j + 2];
                t3 = A[k][j + 3];
                t4 = A[k][j + 4];
                t5 = A[k][j + 5];
                t6 = A[k][j + 6];
                t7 = A[k][j + 7];

                B[j + 0][k] = t0;
                B[j + 1][k] = t1;
                B[j + 2][k] = t2;
                B[j + 3][k] = t3;
                B[j + 0][k + 4] = t4;
                B[j + 1][k + 4] = t5;
                B[j + 2][k + 4] = t6;
                B[j + 3][k + 4] = t7;
            }
            for (int k = i + 4; k < i + 8; ++k) { //后四行，循环四次，A产生4次miss，B产生4次miss
                t0 = A[k][j + 0];
                t1 = A[k][j + 1];
                t2 = A[k][j + 2];
                t3 = A[k][j + 3];
                t4 = A[k][j + 4];
                t5 = A[k][j + 5];
                t6 = A[k][j + 6];
                t7 = A[k][j + 7];

                B[j + 4][k - 4] = t0;
                B[j + 5][k - 4] = t1;
                B[j + 6][k - 4] = t2;
                B[j + 7][k - 4] = t3;
                B[j + 4][k] = t4;
                B[j + 5][k] = t5;
                B[j + 6][k] = t6;
                B[j + 7][k] = t7;
            }

            for (int k = j; k < j + 4; ++k) {  //问题出在这里，B前四行产生4次miss，B后四行产生4次miss
                t0 = B[k][i + 4];
                t1 = B[k][i + 5];
                t2 = B[k][i + 6];
                t3 = B[k][i + 7];

                t4 = B[k + 4][i];
                t5 = B[k + 4][i + 1];
                t6 = B[k + 4][i + 2];
                t7 = B[k + 4][i + 3];

                B[k + 4][i] = t0;
                B[k + 4][i + 1] = t1;
                B[k + 4][i + 2] = t2;
                B[k + 4][i + 3] = t3;

                B[k][i + 4] = t4;
                B[k][i + 5] = t5;
                B[k][i + 6] = t6;
                B[k][i + 7] = t7;
            }
        }
    }
}

//采用8*8分块，但转置的时候又按照4个4*4矩阵转置，同时优化移动顺序
char trans_submit_64_desc[] = "8*8 matrix but trans with 4 4*4 son matrix version，special order";
void transpose_submit_64(int M, int N, int A[N][M], int B[M][N]){
    int t0, t1, t2, t3, t4, t5, t6, t7;
    // 还是8x8分块，这个没有变
    for (int i = 0; i < N; i += 8) { 
		for (int j = 0; j < M; j += 8) {
			//每个8x8分块A产生8次miss，B产生8次miss，总共是16次miss
			for (int k = i; k < i + 4; ++k) { //把a的前四行赋给b的前四行，循环四次，A产生4次miss，B产生4次miss
				t0 = A[k][j + 0];
				t1 = A[k][j + 1];
				t2 = A[k][j + 2];
				t3 = A[k][j + 3];
				t4 = A[k][j + 4];
				t5 = A[k][j + 5];
				t6 = A[k][j + 6];
				t7 = A[k][j + 7];

				B[j + 0][k] = t0;
				B[j + 1][k] = t1;
				B[j + 2][k] = t2;
				B[j + 3][k] = t3;
				B[j + 0][k + 4] = t4;
				B[j + 1][k + 4] = t5;
				B[j + 2][k + 4] = t6;
				B[j + 3][k + 4] = t7;
			}

			for (int k = j; k < j + 4; ++k) {
				//获取B的右上部分
				t0 = B[k][i + 4];
				t1 = B[k][i + 5];
				t2 = B[k][i + 6];
				t3 = B[k][i + 7];

				//获取A的左下部分，这里一次性miss4次
				t4 = A[i + 4][k];
				t5 = A[i + 5][k];
				t6 = A[i + 6][k];
				t7 = A[i + 7][k];

				//将A的左下写入B的右上
				B[k][i + 4] = t4;
				B[k][i + 5] = t5;
				B[k][i + 6] = t6;
				B[k][i + 7] = t7;

				//将之前B右上部分写入左下部分， 这里每次迭代miss一次，总共4次
				B[k + 4][i] = t0;
				B[k + 4][i + 1] = t1;
				B[k + 4][i + 2] = t2;
				B[k + 4][i + 3] = t3;
			}

			for (int k = i + 4; k < i + 8; ++k) {
				//此时A、B的右下都在缓存中，不会miss
				t0 = A[k][j + 4];
				t1 = A[k][j + 5];
				t2 = A[k][j + 6];
				t3 = A[k][j + 7];

				B[j + 4][k] = t0;
				B[j + 5][k] = t1;
				B[j + 6][k] = t2;
				B[j + 7][k] = t3;
			}
		}
	}
}

char trans_submit_61_67_desc[] = "17*17 matrix version";
void transpose_submit_61x67(int M, int N, int A[N][M], int B[M][N]) {
    int size=17;
    int temp;
    for (int n=0; n<N; n+=size) {
        for (int m=0; m<M; m+=size) {
            for (int i=n; i<n+size&&i<N; i++) {
                for (int j=m; j<m+size&&j<M; j++) {
                    temp = A[i][j];
                    B[j][i]=temp;
                }
            }
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(trans, trans_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
	
	registerTransFunction(transpose_test64, trans_test_64_desc);
	
	registerTransFunction(transpose_submit_64, trans_submit_64_desc); 

    registerTransFunction(transpose_submit, transpose_submit_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

