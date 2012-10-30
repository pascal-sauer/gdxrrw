/* rgdx.c
 * code for gdxrrw::rgdx
 * $Id$
 */

#include <R.h>
#include <Rinternals.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "gdxcc.h"
#include "gclgms.h"
#include "globals.h"



/* checkRgdxList: checks the input request list for valid data
 * and updates the read specifier
 */
static void
checkRgdxList (const SEXP lst, rSpec_t *rSpec, int *protectCnt)
{
  SEXP lstNames, tmpUel;
  SEXP bufferUel;
  SEXP compressExp = NULL;      /* from input requestList */
  SEXP dimExp = NULL;           /* from input requestList */
  SEXP fieldExp = NULL;         /* from input requestList */
  SEXP formExp = NULL;          /* from input requestList */
  SEXP nameExp = NULL;          /* from input requestList */
  SEXP teExp = NULL;            /* from input requestList */
  SEXP tsExp = NULL;            /* from input requestList */
  SEXP uelsExp = NULL;          /* from input requestList */
  int i, j;
  int nElements;                /* number of elements in lst */
  const char *tmpName;
  const char *elmtName;         /* list element name */
  Rboolean compress = NA_LOGICAL;

  nElements = length(lst);
  /* check maximum number of elements */
  if (nElements < 1 || nElements > 7) {
    error("Incorrect number of elements in input list argument.");
  }

  lstNames = getAttrib(lst, R_NamesSymbol);

  if (lstNames == R_NilValue) {
    Rprintf("Input list must be named\n");
    Rprintf("Valid names are: 'name', 'dim', 'uels', 'form', 'compress', 'field', 'te', 'ts'.\n");
    error("Please try again with named input list.");
  }

  /* first, check that all names are recognized, reject o/w
   * in the process, store the symbol for direct access later
   */
  for (i = 0;  i < nElements;  i++) {
    elmtName = CHAR(STRING_ELT(lstNames, i));
    if      (strcmp("compress", elmtName) == 0) {
      compressExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("dim", elmtName) == 0) {
      dimExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("field", elmtName) == 0) {
      fieldExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("form", elmtName) == 0) {
      formExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("name", elmtName) == 0) {
      nameExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("te", elmtName) == 0) {
      teExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("ts", elmtName) == 0) {
      tsExp = VECTOR_ELT(lst, i);
    }
    else if (strcmp("uels", elmtName) == 0) {
      uelsExp = VECTOR_ELT(lst, i);
    }
    else {
      Rprintf ("Input list elements for rgdx must be according to this specification:\n");
      Rprintf ("'name', 'dim', 'uels', 'form', 'compress', 'field', 'te', 'ts'.\n");
      error("Incorrect type of rgdx input list element '%s' specified.",
            elmtName);
    }
  }

  /* now process the fields provided */

  if (compressExp) {
    if (TYPEOF(compressExp) == STRSXP) {
      tmpName = CHAR(STRING_ELT(compressExp, 0));
      if (0 == strcasecmp("true", tmpName)) {
        rSpec->compress = 1;
      }
      else if (0 == strcasecmp("false", tmpName)) {
        rSpec->compress = 0;
      }
      else {
        error("Input list element 'compress' must be either 'true' or 'false'.");
      }
    }
    else if (TYPEOF(compressExp) == LGLSXP) {
      compress = LOGICAL(compressExp)[0];
      if (compress == TRUE) {
        rSpec->compress = 1;
      }
      else {
        rSpec->compress = 0;
      }
    }
    else {
      Rprintf ("List element 'compress' must be either string or logical - found %d instead\n",
               TYPEOF(compressExp));
      error("Input list element 'compress' must be either string or logical");
    }
  } /* if compressExp */

  if (dimExp) {
    if (INTSXP == TYPEOF(dimExp)) {
      if (length(dimExp) != 1) {
        error ("Optional input list element 'dim' must have only one element.");
      }
      if (INTEGER(dimExp)[0] < 0) {
        error("Negative value is not allowed as valid input for 'dim'.");
      }
      rSpec->dim = INTEGER(dimExp)[0];
    }
    else if (REALSXP == TYPEOF(dimExp)) {
      if (length(dimExp) != 1) {
        error ("Optional input list element 'dim' must have only one element.");
      }
      if (REAL(dimExp)[0] < 0) {
        error("Negative value is not allowed as valid input for 'dim'.");
      }
      rSpec->dim = (int) REAL(dimExp)[0];
      if (REAL(dimExp)[0] != rSpec->dim) {
        error("Non-integer value is not allowed as valid input for 'dim'.");
      }
    }
    else {
      Rprintf ("List element 'dim' must be numeric - found %d instead\n",
               TYPEOF(dimExp));
      error ("Optional input list element 'dim' must be numeric.");
    }
  } /* dimExp */

  if (fieldExp) {
    char fieldErrorMsg[] = "Input list element 'field' must be in"
      " ['l','m','lo','up','s','all'].";
    if (TYPEOF(fieldExp) != STRSXP ) {
      Rprintf ("List element 'field' must be a string - found %d instead\n",
               TYPEOF(fieldExp));
      error("Input list element 'field' must be string");
    }
    tmpName = CHAR(STRING_ELT(fieldExp, 0));
    if (strlen(tmpName) == 0) {
      error(fieldErrorMsg);
    }
    rSpec->withField = 1;
    if      (0 == strcasecmp("l", tmpName)) {
      rSpec->dField = level;
    }
    else if (0 == strcasecmp("m", tmpName)) {
      rSpec->dField = marginal;
    }
    else if (0 == strcasecmp("lo", tmpName)) {
      rSpec->dField = lower;
    }
    else if (0 == strcasecmp("up", tmpName)) {
      rSpec->dField = upper;
    }
    else if (0 == strcasecmp("s", tmpName)) {
      rSpec->dField = scale;
    }
    else if (0 == strcasecmp("all", tmpName)) {
      rSpec->dField = all;
    }
    else {
      error(fieldErrorMsg);
     }
  } /* if fieldExp */

  if (formExp) {
    if (STRSXP != TYPEOF(formExp)) {
      Rprintf ("List element 'form' must be a string - found %d instead\n",
               TYPEOF(formExp));
      error ("Input list element 'form' must be string");
    }
    tmpName = CHAR(STRING_ELT(formExp, 0));
    if (strcasecmp("full", tmpName) == 0) {
      rSpec->dForm = full;
    }
    else if (strcasecmp("sparse", tmpName) == 0) {
      rSpec->dForm = sparse;
    }
    else {
      error("Input list element 'form' must be either 'full' or 'sparse'.");
    }
  } /* formExp */

  if (NULL == nameExp)
    error ("Required list element 'name' is missing. Please try again." );
  if (TYPEOF(nameExp) != STRSXP) {
    Rprintf ("List element 'name' must be a string - found %d instead\n",
             TYPEOF(nameExp));
    error("Input list element 'name' must be string.");
  }
  tmpName = CHAR(STRING_ELT(nameExp, 0));
  checkStringLength (tmpName);
  strcpy (rSpec->name, tmpName);

  if (teExp) {
    if (TYPEOF(teExp) == STRSXP ) {
      tmpName = CHAR(STRING_ELT(teExp, 0));
      if (strlen(tmpName) == 0) {
        error("Input list element 'te' must be either 'true' or 'false'.");
      }
      if (0 == strcasecmp("true", tmpName)) {
        rSpec->te = 1;
      }
      else if (0 == strcasecmp("false", tmpName)) {
        rSpec->te = 0;
      }
      else {
        error("Input list element 'te' must be either 'true' or 'false'.");
      }
    }
    else if (TYPEOF(teExp) == LGLSXP) {
      if (LOGICAL(teExp)[0] == TRUE) {
        rSpec->te = 1;
      }
    }
    else {
      error("Input list element 'te' must be either string or logical"
            " - found %d instead", TYPEOF(teExp));
    }
  } /* teExp */

  if (tsExp) {
    if (TYPEOF(tsExp) == STRSXP ) {
      tmpName = CHAR(STRING_ELT(tsExp, 0));
      if (0 == strcasecmp("true", tmpName)) {
        rSpec->ts = 1;
      }
      else if (0 == strcasecmp("false", tmpName)) {
        rSpec->ts = 0;
      }
      else {
        error("Input list element 'ts' must be either 'true' or 'false'.");
      }
    }
    else if (TYPEOF(tsExp) == LGLSXP) {
      if (LOGICAL(tsExp)[0] == TRUE) {
        rSpec->ts = 1;
      }
    }
    else {
      Rprintf ("List element 'ts' must be either string or logical"
               " - found %d instead\n", TYPEOF(tsExp));
      error("Input list element 'ts' must be either string or logical");
    }
  } /* tsExp */

  if (uelsExp) {
    if (TYPEOF(uelsExp) != VECSXP) {
      error("List element 'uels' must be a list.");
    }
    else {
      PROTECT(rSpec->filterUel = allocVector(VECSXP, length(uelsExp)));
      ++*protectCnt;
      for (j = 0; j < length(uelsExp); j++) {
        tmpUel = VECTOR_ELT(uelsExp, j);
        if (tmpUel == R_NilValue) {
          error("Empty Uel is not allowed");
        }
        else {
          PROTECT(bufferUel = allocVector(STRSXP, length(tmpUel)));
          /* Convert to output */
          makeStrVec (bufferUel, tmpUel);
          SET_VECTOR_ELT(rSpec->filterUel, j, bufferUel);
          UNPROTECT(1);         /* bufferUel */
        }
      }
      rSpec->withUel = 1;
    }
  } /* uelsExp */
} /* checkRgdxList */



/* rgdx: gateway function for reading gdx, called from R via .External
 * first argument <- gdx file name
 * second argument <- requestList containing several elements
 * that make up a read specifier, e.g. symbol name, dim, form, etc
 * third argument <- squeeze specifier
 * fourth argument <- useDomInfo specifier
 * ------------------------------------------------------------------ */
SEXP rgdx (SEXP args)
{
  const char *funcName = "rgdx";
  SEXP fileName, requestList, squeeze, udi, universe;
  SEXP outName = R_NilValue,
    outType = R_NilValue,
    outDim = R_NilValue,
    outValSp = R_NilValue,      /* output .val, sparse form */
    outValFull = R_NilValue,    /* output .val, full form */
    outForm = R_NilValue,
    outUels = R_NilValue,
    outField = R_NilValue,
    outTs = R_NilValue,
    outTeSp = R_NilValue,       /* output .te, sparse form */
    outTeFull = R_NilValue,     /* output .te, full form */
    outDomains = R_NilValue;    /* output .domains */
  SEXP outListNames, outList, dimVect, dimNames;
  hpFilter_t hpFilter[GMS_MAX_INDEX_DIM];
  xpFilter_t xpFilter[GMS_MAX_INDEX_DIM];
  int outIdx[GMS_MAX_INDEX_DIM];
  FILE    *fin;
  rSpec_t *rSpec;
  gdxUelIndex_t uels;
  gdxValues_t values;
  gdxSVals_t sVals;
  d64_t d64;
  double dt, posInf, negInf;
  shortStringBuf_t msgBuf;
  shortStringBuf_t uelName;
  shortStringBuf_t gdxFileName;
  int symIdx, symDim, symType, symNNZ, symUser;
  int symDimX;                  /* allow for additional dim on var/equ with field='all' */
  SEXP fieldUels = R_NilValue; /* UELS for addition dimension for field */
  int iDim;
  int rc, findrc, errNum, nUEL, iUEL;
  int mrows = 0;                /* NNZ count, i.e. number of rows in
                                 * $val when form='sparse' */
  int ncols;                    /* number of cols in $val when form='sparse' */
  int kk, iRec, nRecs, index, changeIdx, kRec;
  int rgdxAlloc;                /* PROTECT count: undo this many on exit */
  int UELUserMapping, highestMappedUEL;
  int foundTuple;
  int arglen, matched;
  double *p, *dimVal;
  char buf[3*sizeof(shortStringBuf_t)];
  char strippedID[GMS_SSSIZE];
  char symName[GMS_SSSIZE];
  char symText[GMS_SSSIZE], msg[GMS_SSSIZE], stringEle[GMS_SSSIZE];
  char *types[] = {"set", "parameter", "variable", "equation"};
  char *forms[] = {"full", "sparse"};
  char *fields[] = {"l", "m", "lo", "up", "s", "all"};
  int nField, elementIndex, IDum, ndimension, totalElement;
  int withList = 0;
  int outElements = 0;    /* shut up compiler warnings */
  int nnz;         /* symbol cardinality, i.e. nonzero count */
  int nnzMax;      /* maximum possible nnz for this symbol */
  Rboolean zeroSqueeze = NA_LOGICAL;
  Rboolean useDomInfo = NA_LOGICAL;

  /* setting initial values */
  rgdxAlloc = 0;

  /* first arg is function name - ignore it */
  arglen = length(args);

  /* ----------------- Check proper number of inputs and outputs ------------
   * Function should follow specification of
   * rgdx ('gdxFileName', requestList = NULL, squeeze = TRUE, useDomInfo=TRUE)
   * ------------------------------------------------------------------------ */
  if (5 != arglen) {
    error ("usage: %s(gdxName, requestList = NULL, squeeze = TRUE, useDomInfo = TRUE) - incorrect arg count", funcName);
  }
  fileName = CADR(args);
  requestList = CADDR(args);
  squeeze = CADDDR(args);
  udi = CAD4R(args);
  if (TYPEOF(fileName) != STRSXP) {
    error ("usage: %s(gdxName, requestList = NULL) - gdxName must be a string", funcName);
  }
  if (TYPEOF(requestList) == NILSXP)
    withList = 0;
  else {
    withList = 1;
    if (TYPEOF(requestList) != VECSXP) {
      error ("usage: %s(gdxName, requestList, squeeze, useDomInfo) - requestList must be a list", funcName);
    }
  }

  (void) CHAR2ShortStr (CHAR(STRING_ELT(fileName, 0)), gdxFileName);

  if (! withList) {
    if (0 == strcmp("?", gdxFileName)) {
      int n = (int)strlen (ID);
      memcpy (strippedID, ID+1, n-2);
      strippedID[n-2] = '\0';
      Rprintf ("R-file source info: %s\n", strippedID);
      return R_NilValue;
    } /* if audit run */
  } /* if one arg, of character type */

  zeroSqueeze = getSqueezeArgRead (squeeze);
  if (NA_LOGICAL == zeroSqueeze) {
    error ("usage: %s(gdxName, requestList, squeeze = TRUE, useDomInfo = TRUE)\n    squeeze argument could not be interpreted as logical", funcName);
  }
  useDomInfo = getSqueezeArgRead (udi);
  if (NA_LOGICAL == useDomInfo) {
    error ("usage: %s(gdxName, requestList, squeeze = TRUE, useDomInfo = TRUE)\n    useDomInfo argument could not be interpreted as logical", funcName);
  }

  /* ------------------- check if the GDX file exists --------------- */
  checkFileExtension (gdxFileName);
  fin = fopen (gdxFileName, "r");
  if (fin==NULL) {
    error ("GDX file '%s' not found", gdxFileName);
  }
  fclose(fin);
  /*-------------------- Checking data for input list ------------*/
  /* Setting default values */
  rSpec = malloc(sizeof(*rSpec));
  memset (rSpec, 0, sizeof(*rSpec));
  rSpec->dForm = sparse;
  rSpec->dField = level;
  rSpec->dim = -1;              /* negative indicates NA */

  memset (hpFilter, 0, sizeof(hpFilter));
  memset (xpFilter, 0, sizeof(xpFilter));

  if (withList) {
    checkRgdxList (requestList, rSpec, &rgdxAlloc);
    if (rSpec->compress && rSpec->withUel) {
      error("Compression is not allowed with input UELs");
    }
  }

  loadGDX();
  rc = gdxCreate (&gdxHandle, msgBuf, sizeof(msgBuf));
  if (0 == rc)
    error ("Error creating GDX object: %s", msgBuf);
  rc = gdxOpenRead (gdxHandle, gdxFileName, &errNum);
  if (errNum || 0 == rc) {
    error("Could not open gdx file with gdxOpenRead");
  }

  gdxGetSpecialValues (gdxHandle, sVals);
  d64.u64 = 0x7fffffffffffffff; /* positive QNaN, mantissa all on */
  sVals[GMS_SVIDX_UNDEF] = d64.x;
  sVals[GMS_SVIDX_NA] = NA_REAL;
  dt = 0.0;
  posInf =  1 / dt;
  negInf = -1 / dt;
  sVals[GMS_SVIDX_EPS] = 0;
  sVals[GMS_SVIDX_PINF] = posInf;
  sVals[GMS_SVIDX_MINF] = negInf;
  gdxSetSpecialValues (gdxHandle, sVals);

  /* read symbol name only if input list is present */
  if (withList) {
    /* start searching for symbol */
    rc = gdxFindSymbol (gdxHandle, rSpec->name, &symIdx);
    if (! rc) {
      sprintf (buf, "GDX file %s contains no symbol named '%s'",
               gdxFileName,
               rSpec->name );
      error (buf);
    }
    gdxSymbolInfo (gdxHandle, symIdx, symName, &symDim, &symType);
    gdxSymbolInfoX (gdxHandle, symIdx, &symNNZ, &symUser, symText);
    /* symNNZ aka nRecs: count of nonzeros/records in symbol */

    switch (symType) {
    case GMS_DT_SET:
      if (rSpec->withField)
        error("Bad read specifier for set symbol '%s': 'field' not allowed.",
              rSpec->name);
      break;
    case GMS_DT_PAR:
      if (rSpec->withField)
        error("Bad read specifier for parameter symbol '%s': 'field' not allowed.",
              rSpec->name);
      break;
    case GMS_DT_VAR:
    case GMS_DT_EQU:
      /* no checks necessary */
      break;
    case GMS_DT_ALIAS:          /* follow link to actual set */
      symIdx = symUser;
      gdxSymbolInfo (gdxHandle, symIdx, symName, &symDim, &symType);
      gdxSymbolInfoX (gdxHandle, symIdx, &symNNZ, &symUser, symText);
      break;
    default:
      sprintf(buf, "GDX symbol %s (index=%d, symDim=%d, symType=%d)"
              " is not recognized as set, parameter, variable, or equation",
              rSpec->name, symIdx, symDim, symType);
      error(buf);
    } /* end switch */

    if (rSpec->te && symType != GMS_DT_SET) {
      error("Text elements only exist for sets and symbol '%s' is not a set.",
            rSpec->name);
    }
    if (rSpec->dim >= 0) {
      /* check that symbol dim agrees with expected dim */
      if (rSpec->dim != symDim) {
        sprintf(buf, "Symbol %s has dimension %d but you specifed dim=%d",
                rSpec->name, symDim, rSpec->dim);
        error(buf);
      }
    }
  } /* if (withList) */

  /* Get UEL universe from GDX file */
  (void) gdxUMUelInfo (gdxHandle, &nUEL, &highestMappedUEL);
  PROTECT(universe = allocVector(STRSXP, nUEL));
  rgdxAlloc++;
  for (iUEL = 1;  iUEL <= nUEL;  iUEL++) {
    if (!gdxUMUelGet (gdxHandle, iUEL, uelName, &UELUserMapping)) {
      error("Could not gdxUMUelGet");
    }
    SET_STRING_ELT(universe, iUEL-1, mkChar(uelName));
  }

  outElements = 6;   /* outList has at least 6 elements, maybe more */
  if (withList) { /* aa */
    /* Checking dimension of input uel and parameter in GDX file.
     * If they are not equal then error. */
    if (rSpec->withUel && length(rSpec->filterUel) != symDim) {
      error("Dimension of UEL filter entered does not match with symbol in GDX");
    }
    /* initialize hpFilter to use a universe filter for each dimension */
    for (iDim = 0;  iDim < symDim;  iDim++) {
      hpFilter[iDim].fType = identity;
    }

    ncols = symDim + 1;         /* usual index cols + data col */
    symDimX = symDim;
    switch (symType) {
    case GMS_DT_SET:
      ncols = symDim;           /* no data col */
      break;
    case GMS_DT_VAR:
    case GMS_DT_EQU:
      if (all == rSpec->dField) { /* additional 'field' col */
        ncols++;
        symDimX++;
        PROTECT(fieldUels = allocVector(STRSXP, GMS_VAL_MAX));
        rgdxAlloc++;
        SET_STRING_ELT(fieldUels, GMS_VAL_LEVEL   , mkChar(fields[GMS_VAL_LEVEL   ]));
        SET_STRING_ELT(fieldUels, GMS_VAL_MARGINAL, mkChar(fields[GMS_VAL_MARGINAL]));
        SET_STRING_ELT(fieldUels, GMS_VAL_LOWER   , mkChar(fields[GMS_VAL_LOWER   ]));
        SET_STRING_ELT(fieldUels, GMS_VAL_UPPER   , mkChar(fields[GMS_VAL_UPPER   ]));
        SET_STRING_ELT(fieldUels, GMS_VAL_SCALE   , mkChar(fields[GMS_VAL_SCALE   ]));
      }
      break;
    } /* end switch */

    /* we will have domain info returned for all symbols */
    PROTECT(outDomains = allocVector(STRSXP, symDimX));
    rgdxAlloc++;

    outTeSp = R_NilValue;
    nnz = 0;
    if (rSpec->withUel) {
      if (all == rSpec->dField) {
        error ("field='all' not yet implemented: 000");
      }

      /* here we check the cardinality of the symbol we are reading,
       * i.e. the number of nonzeros, i.e. the number of elements that match
       * in uel filter.  Given this value,
       * we can create a 2D double matrix for sparse format.
       */
      /* create integer filters */
      for (iDim = 0;  iDim < symDim;  iDim++) {
        mkHPFilter (VECTOR_ELT(rSpec->filterUel, iDim), hpFilter + iDim);
      }
      for (nnzMax = 1, iDim = 0;  iDim < symDim;  iDim++) {
        nnzMax *=  length(VECTOR_ELT(rSpec->filterUel, iDim));
      }

      /* set domain names to "_user": cannot conflict with real set names */
      for (iDim = 0;  iDim < symDim;  iDim++) {
        SET_STRING_ELT(outDomains, iDim, mkChar("_user"));
      }

      /* Start reading data */
      gdxDataReadRawStart (gdxHandle, symIdx, &nRecs);
      prepHPFilter (symDim, hpFilter);
      for (iRec = 0;  iRec < nRecs;  iRec++) {
        gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
        foundTuple = findInHPFilter (symDim, uels, hpFilter, outIdx);
        if (foundTuple) {
          nnz++;
          if (nnz >= nnzMax) {
            break;
          }
        }
      } /* loop over gdx records */
      if (!gdxDataReadDone (gdxHandle)) {
        error ("Could not gdxDataReadDone");
      }

      /* Allocating memory for 2D sparse matrix */
      PROTECT(outValSp = allocMatrix(REALSXP, nnz, ncols));
      rgdxAlloc++;
      p = REAL(outValSp);

      if (rSpec->te) { /* read set elements with their text, using filter */
        PROTECT(outTeSp = allocVector(STRSXP, nnz));
        rgdxAlloc++;
        gdxDataReadRawStart (gdxHandle, symIdx, &nRecs);
        prepHPFilter (symDim, hpFilter);
        for (matched = 0, iRec = 0;  iRec < nRecs;  iRec++) {
          gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
          foundTuple = findInHPFilter (symDim, uels, hpFilter, outIdx);
          if (foundTuple) {
            for (iDim = 0;  iDim < symDim;  iDim++) {
              p[matched + iDim*nnz] = outIdx[iDim];
            }

            index = matched + symDim * nnz;

            if (values[GMS_VAL_LEVEL]) {
              elementIndex = (int) values[GMS_VAL_LEVEL];
              gdxGetElemText(gdxHandle, elementIndex, msg, &IDum);
              SET_STRING_ELT(outTeSp, matched, mkChar(msg));
            }
            else {
              strcpy(stringEle, "");
              for (iDim = 0;  iDim < symDim;  iDim++) {
                strcat(stringEle, CHAR(STRING_ELT(universe, uels[iDim]-1)));
                if (iDim != symDim-1) {
                  strcat(stringEle, ".");
                }
              }
              SET_STRING_ELT(outTeSp, matched, mkChar(stringEle));
            }
            matched++;
          }
          if (matched == nnz) {
            break;
          }
        }  /* loop over GDX records */
        if (!gdxDataReadDone (gdxHandle)) {
          error ("Could not gdxDataReadDone");
        }
      } /* if rSpec->te */
      else {
        gdxDataReadRawStart (gdxHandle, symIdx, &nRecs);
        prepHPFilter (symDim, hpFilter);
        for (matched = 0, iRec = 0;  iRec < nRecs;  iRec++) {
          gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
          foundTuple = findInHPFilter (symDim, uels, hpFilter, outIdx);
          if (foundTuple) {
            for (iDim = 0;  iDim < symDim;  iDim++) {
              p[matched + iDim*nnz] = outIdx[iDim];
            }
            index = matched + symDim * nnz;
            matched++;

            if (symType != GMS_DT_SET) {
              if (all == rSpec->dField) {
                error ("field='all' not yet implemented: 100");
              }
              else {
                p[index] = values[rSpec->dField];
              }
            }
          }
          if (matched == nnz) {
            break;
          }
        } /* loop over GDX records */
        if (!gdxDataReadDone (gdxHandle)) {
          error ("Could not gdxDataReadDone");
        }
      } /* if (te) .. else .. */
    }   /* if withUel */
    else {
      /* read without user UEL filter: use domain info to filter if possible */
      mrows = symNNZ;
      /*  check for non zero elements for variable and equation */
      if ((symType == GMS_DT_VAR || symType == GMS_DT_EQU)) {
        if (all == rSpec->dField) {
          mrows *= 5;           /* l,m,lo,up,scale */
        }
        else if (zeroSqueeze) { /* potentially squeeze some out */
          mrows = getNonZeroElements(gdxHandle, symIdx, rSpec->dField);
        }
      }
      /* Create 2D sparse R array */
      PROTECT(outValSp = allocMatrix(REALSXP, mrows, ncols));
      rgdxAlloc++;
      p = REAL(outValSp);

      mkXPFilter (symIdx, useDomInfo, xpFilter, outDomains);

      kRec = 0;                 /* shut up warnings */
      gdxDataReadRawStart (gdxHandle, symIdx, &nRecs);
      switch (symType) {
      case GMS_DT_SET:
        if (rSpec->te) {
          PROTECT(outTeSp = allocVector(STRSXP, mrows));
          rgdxAlloc++;
        }
        for (iRec = 0;  iRec < nRecs;  iRec++) {
          gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
          findrc = findInXPFilter (symDim, uels, xpFilter, outIdx);
          if (findrc) {
            error ("DEBUG 00: findrc = %d is unhandled", findrc);
          }
          for (kk = 0;  kk < symDim;  kk++) {
            p[iRec + kk*mrows] = outIdx[kk]; /* from the xpFilter */
          }
          if (rSpec->te) {
            if (values[GMS_VAL_LEVEL]) {
              elementIndex = (int) values[GMS_VAL_LEVEL];
              gdxGetElemText(gdxHandle, elementIndex, msg, &IDum);
              SET_STRING_ELT(outTeSp, iRec, mkChar(msg));
            }
            else {
              strcpy(stringEle, "");
              for (kk = 0;  kk < symDim;  kk++) {
                strcat(stringEle, CHAR(STRING_ELT(universe, uels[kk]-1)));
                if (kk != symDim-1) {
                  strcat(stringEle, ".");
                }
              }
              SET_STRING_ELT(outTeSp, iRec, mkChar(stringEle));
            }
          } /* if returning set text */
        } /* loop over GDX records */
        kRec = nRecs;
        break;
      case GMS_DT_PAR:
        for (iRec = 0, kRec = 0;  iRec < nRecs;  iRec++) {
          gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
          findrc = findInXPFilter (symDim, uels, xpFilter, outIdx);
          if (findrc) {
            error ("DEBUG 00: findrc = %d is unhandled", findrc);
          }
          if ((! zeroSqueeze) ||
              (0 != values[GMS_VAL_LEVEL])) {
            /* store the value */
            for (index = kRec, kk = 0;  kk < symDim;  kk++) {
              p[index] = outIdx[kk]; /* from the xpFilter */
              index += mrows;
            }
            p[index] = values[GMS_VAL_LEVEL];
            kRec++;
          } /* end if (no squeeze || val != 0) */
        } /* loop over GDX records */
        break;
      case GMS_DT_VAR:
      case GMS_DT_EQU:
        if (all != rSpec->dField) {
          for (iRec = 0, kRec = 0;  iRec < nRecs;  iRec++) {
            gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
            findrc = findInXPFilter (symDim, uels, xpFilter, outIdx);
            if (findrc) {
              error ("DEBUG 00: findrc = %d is unhandled", findrc);
            }
            if ((! zeroSqueeze) ||
                (0 != values[rSpec->dField])) {
              /* store the value */
              for (index = kRec, kk = 0;  kk < symDim;  kk++) {
                p[index] = outIdx[kk]; /* from the xpFilter */
                index += mrows;
              }
              p[index] = values[rSpec->dField];
              kRec++;
            } /* end if (no squeeze || val != 0) */
          } /* loop over GDX records */
        }
        else {
          for (iRec = 0, kRec = 0;  iRec < nRecs;  iRec++) {
            gdxDataReadRaw (gdxHandle, uels, values, &changeIdx);
            findrc = findInXPFilter (symDim, uels, xpFilter, outIdx);
            if (findrc) {
              error ("DEBUG 00: findrc = %d is unhandled", findrc);
            }
            for (index = kRec, kk = 0;  kk < symDim;  kk++) {
              p[index+GMS_VAL_LEVEL   ] = outIdx[kk];
              p[index+GMS_VAL_MARGINAL] = outIdx[kk];
              p[index+GMS_VAL_LOWER   ] = outIdx[kk];
              p[index+GMS_VAL_UPPER   ] = outIdx[kk];
              p[index+GMS_VAL_SCALE   ] = outIdx[kk];
              index += mrows;
            }
            p[index+GMS_VAL_LEVEL   ] = 1 + GMS_VAL_LEVEL;
            p[index+GMS_VAL_MARGINAL] = 1 + GMS_VAL_MARGINAL;
            p[index+GMS_VAL_LOWER   ] = 1 + GMS_VAL_LOWER;
            p[index+GMS_VAL_UPPER   ] = 1 + GMS_VAL_UPPER;
            p[index+GMS_VAL_SCALE   ] = 1 + GMS_VAL_SCALE;
            index += mrows;
            p[index+GMS_VAL_LEVEL   ] = values[GMS_VAL_LEVEL];
            p[index+GMS_VAL_MARGINAL] = values[GMS_VAL_MARGINAL];
            p[index+GMS_VAL_LOWER   ] = values[GMS_VAL_LOWER];
            p[index+GMS_VAL_UPPER   ] = values[GMS_VAL_UPPER];
            p[index+GMS_VAL_SCALE   ] = values[GMS_VAL_SCALE];
            kRec += GMS_VAL_MAX;
          } /* loop over GDX records */
        }
        break;
      default:
        error("Unrecognized type of symbol found.");
      } /* end switch(symType) */
      if (!gdxDataReadDone (gdxHandle)) {
        error ("Could not gdxDataReadDone");
      }
      if (kRec < mrows) {
        SEXP newCV, tmp;
        double *newp;
        double *from, *to;

        PROTECT(newCV = allocMatrix(REALSXP, kRec, ncols));
        newp = REAL(newCV);
        for (kk = 0;  kk <= symDim;  kk++) {
          from = p    + kk*mrows;
          to   = newp + kk*kRec;
          MEMCPY (to, from, sizeof(*p)*kRec);
        }
        tmp = outValSp;
        outValSp = newCV;
        UNPROTECT_PTR(tmp);
        mrows = kRec;
      }
    } /* if (withUel .. else .. ) */

    /* here the output uels $uels are allocated and populated */
    if (rSpec->compress) {
      PROTECT(outUels = allocVector(VECSXP, symDimX));
      rgdxAlloc++;
      compressData (symDim, mrows, universe, nUEL, xpFilter,
                    outValSp, outUels);
      /* set domain names to "_compressed": cannot conflict with real set names */
      for (iDim = 0;  iDim < symDim;  iDim++) {
        SET_STRING_ELT(outDomains, iDim, mkChar("_compressed"));
      }
    }
    else if (! rSpec->withUel) {
      PROTECT(outUels = allocVector(VECSXP, symDimX));
      rgdxAlloc++;
      xpFilterToUels (symDim, xpFilter, universe, outUels);
    }
    if (all == rSpec->dField) {
      SET_VECTOR_ELT(outUels, symDim, fieldUels);
      SET_STRING_ELT(outDomains, symDim, mkChar("_field"));
    }

    /* Converting sparse data into full matrix */
    if (rSpec->dForm == full) {
      switch (symDim) {
      case 0:
        if (all == rSpec->dField) {
          error ("field='all' not yet implemented: 400");
        }
        PROTECT(outValFull = allocVector(REALSXP, 1));
        rgdxAlloc++;
        if (outValSp != R_NilValue && (REAL(outValSp) != NULL) && (mrows > 0)) {
          REAL(outValFull)[0] = REAL(outValSp)[0];
        }
        else {
          REAL(outValFull)[0] = 0;
        }
        /* sets cannot have symDim 0, so skip conversion of set text */
        break;

      case 1:
        if (all == rSpec->dField) {
          error ("field='all' not yet implemented: 500");
        }
        PROTECT(dimVect = allocVector(REALSXP, 2));
        rgdxAlloc++;
        dimVal = REAL(dimVect);
        dimVal[1] = 1;
        PROTECT(dimNames = allocVector(VECSXP, 2)); /* for one-dim symbol, val is 2-dim */
        rgdxAlloc++;
        SET_VECTOR_ELT(dimNames, 1, R_NilValue); /* no names for 2nd dimension */

        if (rSpec->withUel) {
          dimVal[0] = length(VECTOR_ELT(rSpec->filterUel, 0));
          PROTECT(outValFull = allocVector(REALSXP, dimVal[0]));
          rgdxAlloc++;
          sparseToFull (outValSp, outValFull, rSpec->filterUel, symType, nnz, symDim);
          setAttrib(outValFull, R_DimSymbol, dimVect);
          SET_VECTOR_ELT(dimNames, 0, VECTOR_ELT(rSpec->filterUel, 0));
          setAttrib(outValFull, R_DimNamesSymbol, dimNames);
        }
        else {
          dimVal[0] = length(VECTOR_ELT(outUels, 0));
          PROTECT(outValFull = allocVector(REALSXP, dimVal[0]));
          rgdxAlloc++;
          sparseToFull (outValSp, outValFull, outUels, symType, mrows, symDim);
          setAttrib(outValFull, R_DimSymbol, dimVect);
          SET_VECTOR_ELT(dimNames, 0, VECTOR_ELT(outUels, 0));
          setAttrib(outValFull, R_DimNamesSymbol, dimNames);
        }

        if (rSpec->te) {   /* create full dimensional string matrix */
          PROTECT(outTeFull = allocVector(STRSXP, dimVal[0]));
          rgdxAlloc++;
          if (rSpec->withUel) {
            createElementMatrix (outValSp, outTeSp, outTeFull, rSpec->filterUel, symDim, nnz);
          }
          else {
            createElementMatrix (outValSp, outTeSp, outTeFull, outUels, symDim, mrows);
          }
          setAttrib(outTeFull, R_DimSymbol, dimVect); /* .te has same dimension as .val */
          setAttrib(outTeFull, R_DimNamesSymbol, dimNames);
        } /* if rSpec->te */
        break;

      default:
        PROTECT(dimVect = allocVector(REALSXP, symDimX));
        rgdxAlloc++;
        totalElement = 1;
        dimVal = REAL(dimVect);
        if (rSpec->withUel) {
          if (all == rSpec->dField) {
            error ("field='all' not yet implemented: 600");
          }
          for (ndimension = 0; ndimension < symDim; ndimension++) {
            dimVal[ndimension] = length(VECTOR_ELT(rSpec->filterUel, ndimension));
            totalElement *= dimVal[ndimension];
          }
        }
        else {
          for (ndimension = 0; ndimension < symDimX; ndimension++) {
            dimVal[ndimension] = length(VECTOR_ELT(outUels, ndimension));
            totalElement *= dimVal[ndimension];
          }
        }
        PROTECT(outValFull = allocVector(REALSXP, totalElement));
        rgdxAlloc++;
        if (rSpec->withUel) {
          sparseToFull (outValSp, outValFull, rSpec->filterUel, symType, nnz, symDim);
          setAttrib(outValFull, R_DimSymbol, dimVect);
          setAttrib(outValFull, R_DimNamesSymbol, rSpec->filterUel);
        }
        else {
          sparseToFull (outValSp, outValFull, outUels, symType, mrows, symDimX);
          setAttrib(outValFull, R_DimSymbol, dimVect);
          setAttrib(outValFull, R_DimNamesSymbol, outUels);
        }

        if (rSpec->te) {   /* create full dimensional string matrix */
          PROTECT(outTeFull = allocVector(STRSXP, totalElement));
          rgdxAlloc++;
          if (rSpec->withUel) {
            createElementMatrix (outValSp, outTeSp, outTeFull, rSpec->filterUel, symDim, nnz);
            setAttrib(outTeFull, R_DimSymbol, dimVect);
            setAttrib(outTeFull, R_DimNamesSymbol, rSpec->filterUel);
          }
          else {
            createElementMatrix (outValSp, outTeSp, outTeFull, outUels, symDim, mrows);
            setAttrib(outTeFull, R_DimSymbol, dimVect);
            setAttrib(outTeFull, R_DimNamesSymbol, outUels);
          }
        } /* if rSpec->te */
        break;
      } /* switch(symDim) */
    }   /* if dForm = full */

    /* Creating output string for symbol name */
    PROTECT(outName = allocVector(STRSXP, 1) );
    SET_STRING_ELT(outName, 0, mkChar(symName));
    rgdxAlloc++;
    /* Creating output string for symbol type */
    PROTECT(outType = allocVector(STRSXP, 1) );
    rgdxAlloc++;
    switch (symType) {
    case GMS_DT_SET:
      SET_STRING_ELT(outType, 0, mkChar(types[0]) );
      break;
    case GMS_DT_PAR:
      SET_STRING_ELT(outType, 0, mkChar(types[1]) );
      break;
    case GMS_DT_VAR:
      SET_STRING_ELT(outType, 0, mkChar(types[2]) );
      break;
    case GMS_DT_EQU:
      SET_STRING_ELT(outType, 0, mkChar(types[3]) );
      break;
    default:
      error("Unrecognized type of symbol found.");
    }

    /* Creating int vector for symbol dim */
    PROTECT(outDim = allocVector(INTSXP, 1) );
    INTEGER(outDim)[0] = symDim;
    rgdxAlloc++;
    /* Creating string vector for val data form */
    PROTECT(outForm = allocVector(STRSXP, 1) );
    rgdxAlloc++;
    if (rSpec->dForm == full) {
      SET_STRING_ELT(outForm, 0, mkChar(forms[0]));
    }
    else {
      SET_STRING_ELT(outForm, 0, mkChar(forms[1]));
    }


    outElements++;       /* for domains */
    /* Create a string vector for symbol field */
    if (symType == GMS_DT_VAR || symType == GMS_DT_EQU) {
      outElements++;
      PROTECT(outField = allocVector(STRSXP, 1));
      rgdxAlloc++;
      switch(rSpec->dField) {
      case level:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_LEVEL]));
        break;
      case marginal:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_MARGINAL]));
        break;
      case lower:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_LOWER]));
        break;
      case upper:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_UPPER]));
        break;
      case scale:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_SCALE]));
        break;
      case all:
        SET_STRING_ELT(outField, 0, mkChar (fields[GMS_VAL_MAX]));
        break;
      default:
        error("Unrecognized type of field found.");
      }
    }
    if (rSpec->ts) {
      outElements++;
      PROTECT(outTs = allocVector(STRSXP, 1));
      rgdxAlloc++;
      SET_STRING_ELT(outTs, 0, mkChar(symText));
    }
    if (rSpec->te) {
      outElements++;
    }
  } /* if (withList) aa */
  else {
    /* no requestList was input, so returning universe */
    /* Creating output string symbol name */
    PROTECT(outName = allocVector(STRSXP, 1));
    SET_STRING_ELT(outName, 0, mkChar("*"));
    rgdxAlloc++;
    /* Creating output string for symbol type */
    PROTECT(outType = allocVector(STRSXP, 1));
    rgdxAlloc++;
    SET_STRING_ELT(outType, 0, mkChar(types[0]));
    /* Creating int vector for symbol dim */
    PROTECT(outDim = allocVector(INTSXP, 1));
    INTEGER(outDim)[0] = 1;
    rgdxAlloc++;
  }

  PROTECT(outListNames = allocVector(STRSXP, outElements));
  rgdxAlloc++;
  /* populating list element names */
  SET_STRING_ELT(outListNames, 0, mkChar("name"));
  SET_STRING_ELT(outListNames, 1, mkChar("type"));
  SET_STRING_ELT(outListNames, 2, mkChar("dim"));
  SET_STRING_ELT(outListNames, 3, mkChar("val"));
  SET_STRING_ELT(outListNames, 4, mkChar("form"));
  SET_STRING_ELT(outListNames, 5, mkChar("uels"));
  if (withList) {
    SET_STRING_ELT(outListNames, 6, mkChar("domains"));
    nField = 7;
    if (symType == GMS_DT_VAR || symType == GMS_DT_EQU) {
      SET_STRING_ELT(outListNames, nField, mkChar("field"));
      nField++;
    }
    if (rSpec->ts) {
      SET_STRING_ELT(outListNames, nField, mkChar("ts"));
      nField++;
    }
    if (rSpec->te) {
      SET_STRING_ELT(outListNames, nField, mkChar("te"));
      nField++;
    }
  }

  PROTECT(outList = allocVector(VECSXP, outElements));
  rgdxAlloc++;

  /* populating list component vector */
  SET_VECTOR_ELT(outList, 0, outName);
  SET_VECTOR_ELT(outList, 1, outType);
  SET_VECTOR_ELT(outList, 2, outDim);
  if (withList) {
    if (rSpec->dForm == full) {
      SET_VECTOR_ELT(outList, 3, outValFull);
    }
    else {
      SET_VECTOR_ELT(outList, 3, outValSp);
    }
    SET_VECTOR_ELT(outList, 4, outForm);
    if (rSpec->withUel) {
      SET_VECTOR_ELT(outList, 5, rSpec->filterUel);
    }
    else {
      SET_VECTOR_ELT(outList, 5, outUels);
    }
    SET_VECTOR_ELT(outList, 6, outDomains);

    nField = 7;
    if (symType == GMS_DT_VAR || symType == GMS_DT_EQU) {
      SET_VECTOR_ELT(outList, nField, outField);
      nField++;
    }
    if (rSpec->ts) {
      SET_VECTOR_ELT(outList, nField, outTs);
      nField++;
    }
    if (rSpec->te) {
      if (rSpec->dForm == full) {
        SET_VECTOR_ELT(outList, nField, outTeFull);
      }
      else {
        SET_VECTOR_ELT(outList, nField, outTeSp);
      }
      nField++;
    }
  }
  else {
    /* no read specifier so return the universe */
    /* entering null values if nothing else makes sense */
    SET_VECTOR_ELT(outList, 3, R_NilValue);
    SET_VECTOR_ELT(outList, 4, R_NilValue);
    SET_VECTOR_ELT(outList, 5, universe);
  }

  /* Setting attribute name */
  setAttrib(outList, R_NamesSymbol, outListNames);
  /* Releasing allocated memory */
  free(rSpec);
#if 0
  if (!gdxDataReadDone (gdxHandle)) {
    error ("Could not gdxDataReadDone");
  }
#endif
  errNum = gdxClose (gdxHandle);
  if (errNum != 0) {
    error("Errors detected when closing gdx file");
  }
  (void) gdxFree (&gdxHandle);
  UNPROTECT(rgdxAlloc);
  return outList;
} /* End of rgdx */
