/*! \file
    \ingroup LMP2
    \brief Enter brief description of file here
*/

#include "mpi.h"
#include <iostream>
#include <fstream>              // file I/O support
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//#include <cmath>
//#include <libipv1/ip_lib.h>
#include <libciomr/libciomr.h>
//#include <libchkpt/chkpt.hpp>
//#include <libpsio/psio.h>
#include <libqt/qt.h>
//#include <psifiles.h>
#define EXTERN
#include "globals.h"

namespace psi{

extern int myid;
extern int nprocs;

namespace lmp2{

extern int myid_lmp2;
extern int nprocs_lmp2;

    
LMP2::LMP2() {
  fprintf(outfile, "Do not use the default constructor for LMP2");
}

LMP2::~LMP2() {
//  free_block(D);
}

LMP2::LMP2(shared_ptr<PSIO> psio_o, shared_ptr<Chkpt> chkpt_o) {

  psio = psio_o;
  chkpt = chkpt_o;

}

void LMP2::get_moinfo() {

/*  nso = chkpt->rd_nso();
  natom = chkpt->rd_natom();
  nirreps = chkpt->rd_nirreps();
  nshell = get_nshell();
  C = chkpt->rd_scf();
*/

  nso = get_nso();
  natom = get_natom();
  nirreps = get_nirreps();
  nshell = get_nshell();
  C = get_MOC();

  print_moinfo();

  return;
}

void LMP2::opdm() {

  int i;
  int *doccpi;

  doccpi = get_doccpi();

  for(i=0,nocc=0; i < nirreps; i++) nocc += doccpi[i];

  D = block_matrix(nso,nso);
  C_DGEMM('n', 't', nso, nso, nocc, 1, &(C[0][0]), nso, &(C[0][0]), nso, 0, &(D[0][0]), nso);

}

void LMP2::print_moinfo(){

  // A couple of error traps
  if(nirreps != 1) {
//    if(myid == 0) {
//      fprintf(outfile, "\n\tError: localization is only valid in C1 symmetry!\n");
//    }
    char *symm_label = chkpt->rd_sym_label();
    throw InputException("Local MP2 is only valid in C1 symmetry", symm_label, __FILE__, __LINE__);
//    exit(PSI_RETURN_FAILURE);
  }


  double Enuc = get_enuc();
  double Escf = get_escf();

  if(myid == 0) {
    fprintf(outfile,"\n");
    fprintf(outfile,"\tChkpt Parameters:\n");
    fprintf(outfile,"\t--------------------\n");
    fprintf(outfile,"\tNumber of Irreps    \t= %d\n",nirreps);
    fprintf(outfile,"\tNumber of MOs    \t= %d\n",nso);
    fprintf(outfile,"\n");
    fprintf(outfile,"\tNuclear rep. energy   \t= %20.15f\n",Enuc);
    fprintf(outfile,"\tSCF energy            \t= %20.15f\n",Escf);
    fflush(outfile);
  }
}

void LMP2::get_fock() {

  int i, j;
  double **temp, *X;

  aoF = block_matrix(nso, nso);
  loF = block_matrix(nso, nso);
  X = init_array((nso*nso+nso)/2);
  temp = block_matrix(nso, nso);

  X = chkpt->rd_fock();

  tri_to_sq(X, aoF, nso);
  free(X);

  // Transform the Fock matrix from the AO to LO basis
  C_DGEMM('t', 'n', nso, nso, nso, 1, &(C[0][0]), nso, &(aoF[0][0]), nso, 0, &(temp[0][0]), nso);
  C_DGEMM('n', 'n', nso, nso, nso, 1, &(temp[0][0]), nso, &(C[0][0]), nso, 0, &(loF[0][0]), nso);
  free_block(temp);

}

int* LMP2::get_ij_owner() {

  int i, j, ij, v, count;
  int *ij_owner;

  ij_owner = init_int_array(ij_pairs);

  v=0;
  for(i=0, ij=0; i < nocc; i++) {
    for(j=0; j <= i; j++, ij++) {
      ij_owner[ij] = v%nprocs;
      v++;
    }
  }

  return &(ij_owner[0]);

}

int* LMP2::get_ij_local() {

  int i, j, ij, v, count;
  int *ij_local;

  ij_local = init_int_array(ij_pairs);

  v=0;
  count=0;
  for(i=0, ij=0; i < nocc; i++) {
    for(j=0; j <= i; j++, ij++) {
      ij_local[ij] = count;
      if(v%nprocs == nprocs-1) count++;
      v++;
    }
  }

  return &(ij_local[0]);

}

int* LMP2::get_mn_owner(int n) {

  int count, v, num_unique_shells;
  int *mn_owner;

  num_unique_shells = n;

  mn_owner = init_int_array(n);

  v = 0;
  for(count=0; count < num_unique_shells; count++) {
    mn_owner[count] = v%nprocs;
    v++;
  }

  return &(mn_owner[0]);

}


}} /* End namespace */


