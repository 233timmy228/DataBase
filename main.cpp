
# include<iostream>
# include"p0_starter.h"
using namespace std;
using namespace scudb;

int main() {
    ///
	cout << "SetElement matrixA:" << endl;
    RowMatrix<int> matrixA(3, 4);
    for (int i = 0; i < matrixA.GetRowCount(); i++)
    {
        for (int j = 0; j < matrixA.GetColumnCount(); j++)
        {
            matrixA.SetElement(i, j, i * matrixA.GetColumnCount() + j);
        }
    }
    RowMatrixOperations<int>::Print(matrixA);

    ///
    cout << "Filled matrixB:" << endl;
    RowMatrix<int> matrixB(3, 4);
    vector<int> vecB(12, 2);
    matrixB.FillFrom(vecB);
    RowMatrixOperations<int>::Print(matrixB);

    ///
    cout << "matrixC:" << endl;
    RowMatrix<int> matrixC(4, 3);
    vector<int> vecC(12);
    for (int i = 0; i < 12; i++) {
        vecC[i] = i;
    }
    matrixC.FillFrom(vecC);
    RowMatrixOperations<int>::Print(matrixC);

    ///
    cout << "matrixD:" << endl;
    RowMatrix<int> matrixD(3, 3);
    vector<int> vecD(9);
    for (int i = 0; i < 9; i++) {
        vecD[i] = 9 - i;
    }
    matrixD.FillFrom(vecD);
    RowMatrixOperations<int>::Print(matrixD);

    cout << "Add  matrixA  matrixB:" << endl;
    unique_ptr<RowMatrix<int>> sum;
    sum = RowMatrixOperations<int>::Add(&matrixA, &matrixB);
    RowMatrixOperations<int>::Print(*sum);

    cout << "Multiply matrixB  matrixC" << endl;
    unique_ptr<RowMatrix<int>> mult;
    mult = RowMatrixOperations<int>::Multiply(&matrixB, &matrixC);
    RowMatrixOperations<int>::Print(*mult);

    cout << "GEMM matrixB  matrixC  matrixD" << endl;
    unique_ptr<RowMatrix<int>> gemm;
    gemm = RowMatrixOperations<int>::GEMM(&matrixB, &matrixC, &matrixD);
    RowMatrixOperations<int>::Print(*gemm);

    return 0;
}