
/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "base.h"
#include "stats.h"
#include "sptensor.h"
#include "ftensor.h"
#include "io.h"
#include "reorder.h"

#include <math.h>


/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

/**
* @brief Compute the density of a sparse tensor, defined by nnz/(I*J*K).
*
* @param tt The sparse tensor.
*
* @return The density of tt.
*/
static double __tt_density(
  sptensor_t const * const tt)
{
  double root = pow((double)tt->nnz, 1./(double)tt->nmodes);
  double density = 1.0;
  for(idx_t m=0; m < tt->nmodes; ++m) {
    density *= root / (double)tt->dims[m];
  }

  return density;
}


/**
* @brief Output basic statistics about tt to STDOUT.
*
* @param tt The tensor to inspect.
* @param ifname The filename of tt. Can be NULL.
*/
static void __stats_basic(
  sptensor_t const * const tt,
  char const * const ifname)
{
  printf("Tensor information ---------------------------------------------\n");
  printf("FILE=%s\n", ifname);
  printf("DIMS=%"SS_IDX, tt->dims[0]);
  for(idx_t m=1; m < tt->nmodes; ++m) {
    printf("x%"SS_IDX, tt->dims[m]);
  }
  printf(" NNZ=%"SS_IDX, tt->nnz);
  printf(" DENSITY=%e" , __tt_density(tt));
  printf("\n\n");
}


/**
* @brief Compute statistics about a hypergraph partitioning of tt.
*
* @param tt The tensor to inspect.
* @param mode The mode to operate on.
* @param pfname The file containing the partitioning.
*/
static void __stats_hparts(
  sptensor_t * const tt,
  idx_t const mode,
  char const * const pfname)
{
  if(pfname == NULL) {
    fprintf(stderr, "SPLATT ERROR: analysis type requires partition file\n");
    exit(1);
  }

  ftensor_t * ft = ften_alloc(tt, mode, 0);
  idx_t const nvtxs = ft->nfibs;
  idx_t nhedges = 0;
  for(idx_t m=0; m < tt->nmodes; ++m) {
    nhedges += tt->dims[m];
  }


  idx_t nparts = 0;
  idx_t * parts = part_read(pfname, nvtxs, &nparts);
  idx_t * pptr = NULL;
  idx_t * plookup = NULL;
  build_pptr(parts, nparts, nvtxs, &pptr, &plookup);

  /* get stats on partition sizes */
  idx_t minp = tt->nnz;
  idx_t maxp = 0;
  for(idx_t p=0; p < nparts; ++p) {
    idx_t nnz = 0;
    for(idx_t f=pptr[p]; f < pptr[p+1]; ++f) {
      idx_t const findex = plookup[f];
      nnz += ft->fptr[findex+1] - ft->fptr[findex];
    }
    if(nnz < minp) {
      minp = nnz;
    }
    if(nnz > maxp) {
      maxp = nnz;
    }
  }

  printf("Partition information ------------------------------------------\n");
  printf("FILE=%s\n", pfname);
  printf("NVTXS=%"SS_IDX" NHEDGES=%"SS_IDX"\n", nvtxs, nhedges);
  printf("NPARTS=%"SS_IDX" LIGHTEST=%"SS_IDX" HEAVIEST=%"SS_IDX" AVG=%0.1f\n",
    nparts, minp, maxp, (val_t)(ft->nnz) / (val_t) nparts);
  printf("\n");

  idx_t * unique[MAX_NMODES];
  idx_t nunique[MAX_NMODES];
  for(idx_t m=0; m < ft->nmodes; ++m) {
    unique[m] = (idx_t *) malloc(ft->dims[ft->dim_perms[m]]
      * sizeof(idx_t));
  }

  /* now track unique ind info for each partition */
  for(idx_t p=0; p < nparts; ++p) {
    for(idx_t m=0; m < ft->nmodes; ++m) {
      memset(unique[m], 0, ft->dims[ft->dim_perms[m]] * sizeof(idx_t));
      nunique[m] = 0;
    }

    idx_t nnz = 0;
    idx_t ptr = 0;
    for(idx_t f=pptr[p]; f < pptr[p+1]; ++f) {
      idx_t const findex = plookup[f];
      nnz += ft->fptr[findex+1] - ft->fptr[findex];

      /* find slice of findex */
      while(ft->sptr[ptr] < findex && ft->sptr[ptr+1] < findex) {
        ++ptr;
      }
      if(unique[0][ptr] == 0) {
        ++nunique[0];
        unique[0][ptr] = 1;
      }

      /* mark unique fids */
      if(unique[1][ft->fids[f]] == 0) {
        ++nunique[1];
        unique[1][ft->fids[f]] = 1;
      }

      for(idx_t j=ft->fptr[findex]; j < ft->fptr[findex+1]; ++j) {
        idx_t const jind = ft->inds[j];
        /* mark unique inds */
        if(unique[2][jind] == 0) {
          ++nunique[2];
          unique[2][jind] = 1;
        }
      }
    }

    printf("%"SS_IDX"  ", p);
    printf("fibs: %"SS_IDX"(%4.1f%%)  ", pptr[p+1] - pptr[p],
      100. * (val_t)(pptr[p+1]-pptr[p]) / nvtxs);
    printf("nnz: %"SS_IDX" (%4.1f%%)  ", nnz, 100. * (val_t)nnz / (val_t) tt->nnz);
    printf("I: %"SS_IDX" (%4.1f%%)  ", nunique[0],
      100. * (val_t)nunique[0] / (val_t) ft->dims[mode]);
    printf("K: %"SS_IDX" (%4.1f%%)  ", nunique[1],
      100. * (val_t)nunique[1] / (val_t) ft->dims[ft->dim_perms[1]]);
    printf("J: %"SS_IDX" (%4.1f%%)\n", nunique[2],
      100. * (val_t)nunique[2] / (val_t) ft->dims[ft->dim_perms[2]]);
  }


  for(idx_t m=0; m < ft->nmodes; ++m) {
    free(unique[m]);
  }
  free(parts);
  free(plookup);
  free(pptr);
  ften_free(ft);
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
void stats_tt(
  sptensor_t * const tt,
  char const * const ifname,
  splatt_stats_type const type,
  idx_t const mode,
  char const * const pfile)
{
  switch(type) {
  case STATS_BASIC:
    __stats_basic(tt, ifname);
    break;
  case STATS_HPARTS:
    __stats_hparts(tt, mode, pfile);
    break;
  default:
    fprintf(stderr, "SPLATT ERROR: analysis type not implemented\n");
    exit(1);
  }
}




