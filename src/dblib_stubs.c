/* File: dblib_stubs.c

   Copyright (C) 2010

     Christophe Troestler <Christophe.Troestler@umons.ac.be>
     WWW: http://math.umons.ac.be/an/software/

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License version 3 or
   later as published by the Free Software Foundation.  See the file
   LICENCE for more details.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the file
   LICENSE for more details. */

/* Binding to the DB-Library part of freetds.
   See http://www.freetds.org/userguide/samplecode.htm */

#include <sybfront.h> /* sqlfront.h always comes first */
#include <sybdb.h>
#include <string.h>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>
#include <caml/custom.h>


value ocaml_freetds_dbinit(value unit)
{
  CAMLparam0();
  if (dbinit() == FAIL) {
    failwith("FreeTDS.Dblib: cannot initialize DB-lib!");
  }
  CAMLreturn(Val_unit);
}

/* dberrhandle(err_handler); */
/* dbmsghandle(msg_handler); */


#define DBPROCESS_VAL(v) (* (DBPROCESS **) Data_custom_val(v))
#define DBPROCESS_ALLOC()                                       \
  alloc_custom(&dbprocess_ops, sizeof(DBPROCESS *), 1, 30)

static int dbprocess_compare(value v1, value v2)
{
  /* Compare pointers */
  if (DBPROCESS_VAL(v1) < DBPROCESS_VAL(v2)) return(-1);
  else if (DBPROCESS_VAL(v1) > DBPROCESS_VAL(v2)) return(1);
  else return(0);
}

static long dbprocess_hash(value v)
{
  /* The pointer will do a good hash and respect compare v1 v2 = 0 ==>
     hash(v1) = hash(v2) */
  return((long) DBPROCESS_VAL(v));
}

static struct custom_operations dbprocess_ops = {
  "freetds/dbprocess", /* identifier for serialization and deserialization */
  custom_finalize_default, /* one must call dbclose */
  &dbprocess_compare,
  &dbprocess_hash,
  custom_serialize_default,
  custom_deserialize_default
};


value ocaml_freetds_dbopen(value vuser, value vpasswd, value vserver)
{
  CAMLparam3(vuser, vpasswd, vserver);
  CAMLlocal1(vdbproc);
  LOGINREC *login;
  DBPROCESS *dbproc;

  if ((login = dblogin()) == NULL) {
    failwith("FreeTDS.Dblib.dbopen: cannot allocate the login structure");
  }
  DBSETLUSER(login, String_val(vuser));
  DBSETLPWD(login, String_val(vpasswd));
  if ((dbproc = dbopen(login, String_val(vserver))) == NULL) {
    /* free login ? */
    failwith("FreeTDS.Dblib.dbopen: unable to connect to the database");
  }
  vdbproc = DBPROCESS_ALLOC();
  DBPROCESS_VAL(vdbproc) = dbproc;
  CAMLreturn(vdbproc);
}

value ocaml_freetds_dbclose(value vdbproc)
{
  CAMLparam1(vdbproc);
  dbclose(DBPROCESS_VAL(vdbproc));
  CAMLreturn(Val_unit);
}

value ocaml_freetds_dbuse(value vdbproc, value vdbname)
{
  CAMLparam2(vdbproc, vdbname);
  if (dbuse(DBPROCESS_VAL(vdbproc), String_val(vdbname)) == FAIL) {
    failwith("FreeTDS.Dblib.dbuse: unable to use the given database");
  }
  CAMLreturn(Val_unit);
}

value ocaml_freetds_dbsqlexec(value vdbproc, value vsql)
{
  CAMLparam2(vdbproc, vsql);

  if (dbcmd(DBPROCESS_VAL(vdbproc), String_val(vsql)) == FAIL) {
    failwith("FreeTDS.Dblib.dbsqlexec: cannot allocate memory to hold "
             "the SQL query");
  }
  /* Sending the query to the server resets the command buffer. */
  if (dbsqlexec(DBPROCESS_VAL(vdbproc)) == FAIL) {
    failwith("FreeTDS.Dblib.sqlexec: the SQL query is invalid, the results "
             "of the previous query were not completely read,...");
  }
  CAMLreturn(Val_unit);
}

value ocaml_freetds_dbresults(value vdbproc)
{
  CAMLparam1(vdbproc);
  RETCODE erc;
  if ((erc = dbresults(DBPROCESS_VAL(vdbproc))) == FAIL) {
    failwith("FreeTDS.Dblib.results: query was not processed successfully "
             "by the server");
  }
  CAMLreturn(Val_bool(erc == SUCCEED));
}

value ocaml_freetds_numcols(value vdbproc)
{
  /* noalloc */
  return(Val_int(dbnumcols(DBPROCESS_VAL(vdbproc))));
}

value ocaml_freetds_dbcolname(value vdbproc, value vc)
{
  CAMLparam2(vdbproc, vc);
  CAMLlocal1(vname);
  char *name;
  name = dbcolname(DBPROCESS_VAL(vdbproc), Int_val(vc));
  if (name == NULL)
    invalid_argument("FreeTDS.Dblib.colname: column number out of range");
  vname = caml_copy_string(name);
  /* free(name); */ /* Doing it says "invalid pointer". */
  CAMLreturn(vname);
}

value ocaml_freetds_dbcoltype(value vdbproc, value vc)
{
  CAMLparam2(vdbproc, vc);
  /* Keep in sync with "type col_type" on the Caml side. */
  switch (dbcoltype(DBPROCESS_VAL(vdbproc), Int_val(vc))) {
  case SYBCHAR:    CAMLreturn(Val_int(0));
  case SYBVARCHAR: CAMLreturn(Val_int(1));
  case SYBINTN: CAMLreturn(Val_int(2));
  case SYBINT1: CAMLreturn(Val_int(3));
  case SYBINT2: CAMLreturn(Val_int(4));
  case SYBINT4: CAMLreturn(Val_int(5));
  case SYBINT8: CAMLreturn(Val_int(6));
  case SYBFLT8: CAMLreturn(Val_int(7));
  case SYBFLTN: CAMLreturn(Val_int(8));
  case SYBNUMERIC: CAMLreturn(Val_int(9));
  case SYBDECIMAL: CAMLreturn(Val_int(10));
  case SYBDATETIME: CAMLreturn(Val_int(11));
  case SYBDATETIME4: CAMLreturn(Val_int(12));
  case SYBDATETIMN: CAMLreturn(Val_int(13));
  case SYBBIT: CAMLreturn(Val_int(14));
  case SYBTEXT: CAMLreturn(Val_int(15));
  case SYBIMAGE: CAMLreturn(Val_int(16));
  case SYBMONEY4: CAMLreturn(Val_int(17));
  case SYBMONEY: CAMLreturn(Val_int(18));
  case SYBMONEYN: CAMLreturn(Val_int(19));
  case SYBREAL: CAMLreturn(Val_int(20));
  case SYBBINARY: CAMLreturn(Val_int(21));
  case SYBVARBINARY: CAMLreturn(Val_int(22));
  }
  failwith("Freetds.Dblib.coltype: unknown column type");
}

value ocaml_freetds_dbcancel(value vdbproc)
{
  CAMLparam1(vdbproc);
  dbcancel(DBPROCESS_VAL(vdbproc));
  CAMLreturn(Val_unit);
}

value ocaml_freetds_dbcanquery(value vdbproc)
{
  CAMLparam1(vdbproc);
  dbcanquery(DBPROCESS_VAL(vdbproc));
  CAMLreturn(Val_unit);
}


value ocaml_freetds_dbnextrow(value vdbproc)
{
  CAMLparam1(vdbproc);
  CAMLlocal4(vrow, vdata, vconstructor, vcons);
  DBPROCESS *dbproc = DBPROCESS_VAL(vdbproc);
  int c, ty, converted_len;
  BYTE *data;
  DBINT len;
  int data_int;
  char *data_char;
  double data_double;

/* Taken from the implementation of caml_copy_string */
#define COPY_STRING(res, s, len_bytes)           \
  res = caml_alloc_string(len);                  \
  memmove(String_val(res), s, len_bytes);

#define CONVERT_STRING(destlen)                                         \
  data_char = malloc(destlen); /* printable size */                     \
  converted_len =                                                       \
    dbconvert(dbproc, ty, data, len,  SYBCHAR, (BYTE*) data_char, destlen); \
  if (converted_len < 0) {                                              \
    free(data_char);                                                    \
    failwith("Freetds.nextrow: problem with copying strings. "          \
             "Please contact the author of the Freetds bindings.");      \
  } else {                                                              \
    COPY_STRING(vdata, data, converted_len);                            \
    free(data_char);                                                    \
  }


#define CONSTRUCTOR(tag, value) \
  vconstructor = caml_alloc(1, tag);   \
  Store_field(vconstructor, 0, value)

  switch (dbnextrow(dbproc)) {
  case REG_ROW:
    vrow = Val_int(0); /* empty list [] */
    for (c = dbnumcols(dbproc); c >= 1; c--) {
      data = dbdata(dbproc, c); /* pointer to the data, no copy! */
      len = dbdatlen(dbproc, c); /* length, in bytes, of the data for
                                    a column. */
      if (len == 0) {
        vconstructor = Val_int(0); /* constant constructor NULL */
      } else {
        switch (ty = dbcoltype(dbproc, c)) {
        case SYBCHAR:    /* fall-through */
        case SYBVARCHAR:
        case SYBTEXT:
          COPY_STRING(vdata, data, len);
          CONSTRUCTOR(0, vdata);
          break;
        case SYBIMAGE:
        case SYBBINARY:
        case SYBVARBINARY:
          COPY_STRING(vdata, data, len);
          CONSTRUCTOR(10, vdata);
          break;

        case SYBINT1:
          dbconvert(dbproc, ty, data, len,
                    SYBINT4, (BYTE*) &data_int, sizeof(int));
          CONSTRUCTOR(1, Val_int(data_int));
          break;
        case SYBINT2:
          dbconvert(dbproc, ty, data, len,
                    SYBINT4, (BYTE*) &data_int, sizeof(int));
          CONSTRUCTOR(2, Val_int(data_int));
          break;
        case SYBINTN:
        case SYBINT4:
          data_int = *((int *) data);
#if OCAML_WORD_SIZE == 32
          if (-1073741824 <= data_int && data_int < 1073741824)
            CONSTRUCTOR(3, Val_int(data_int));
          else /* require more than 31 bits allow for */
            CONSTRUCTOR(4, caml_copy_int32(data_int));
#else
          CONSTRUCTOR(3, Val_int(data_int));
#endif
          break;
        case SYBINT8:
          CONVERT_STRING(21);
          CONSTRUCTOR(5, vdata);
          break;

        case SYBFLT8:
          CONSTRUCTOR(6, caml_copy_double(* (double *) data));
          break;
        case SYBFLTN:
        case SYBREAL:
          dbconvert(dbproc, ty, data, len,  SYBFLT8,
                    (BYTE*) &data_double, sizeof(double));
          CONSTRUCTOR(6, caml_copy_double(data_double));
          break;

        case SYBNUMERIC:
          CONVERT_STRING(2.5 * len); /* FIXME: max size ? */
          CONSTRUCTOR(5, vdata);
          break;
        case SYBDECIMAL:
          CONVERT_STRING(2.5 * len); /* FIXME: max size ? */
          CONSTRUCTOR(5, vdata);
          break;

        case SYBBIT:
          CONSTRUCTOR(9, Val_bool(*data != '\0'));
          break;

        case SYBDATETIME:
        case SYBDATETIME4:
        case SYBDATETIMN:
          CONVERT_STRING(128); /* FIXME: max size ? */
          CONSTRUCTOR(7, vdata);
          break;

        case SYBMONEY4:
        case SYBMONEY:
        case SYBMONEYN:
          dbconvert(dbproc, ty, data, len,  SYBFLT8,
                    (BYTE*) &data_double, sizeof(double));
          CONSTRUCTOR(8, caml_copy_double(data_double));
          break;
        }
      }
      /* Place the data in front of the list [vrow]. */
      vcons = alloc_tuple(2);
      Store_field(vcons, 0, vconstructor);
      Store_field(vcons, 1, vrow);
      vrow = vcons;
    }
    CAMLreturn(vrow);
    break;

  case NO_MORE_ROWS:
    caml_raise_not_found();
    break;

  case FAIL:
    failwith("Freetds.Dblib.nextrow");
    break;

  case BUF_FULL:
    failwith("Freetds.Dblib.nextrow: buffer full (report to the developer "
             "of this library)");
    break;

  default:
    /* FIXME: compute rows are ignored */
    CAMLreturn(Val_int(0)); /* row = [] */
  }
}


