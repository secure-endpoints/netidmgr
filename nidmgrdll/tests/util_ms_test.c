/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#include "tests.h"
#include <strsafe.h>

struct string_pair {
  wchar_t * ms;
  wchar_t * csv;
};

struct string_pair strings[] = {
  {L"foo\0bar\0baz,quux\0ab\"cd\0", L"foo,bar,\"baz,quux\",\"ab\"\"cd\""},
  {L"a\0b\0c\0d\0e\0", L"a,b,c,d,e"},
  {L"1\0", L"1"},
  {L"\0", L""},
  {L"b\0a\0", L"b,a"},
  {L"c\0a\0b\0", L"c,a,b"},
  {L"c\0a\0B\0", L"c,a,B"},
  {L"sdf\0Bar\0Foo\0BBB\0", L"sdf,Bar,Foo,BBB"}
};

int n_strings = ARRAYLENGTH(strings);

void print_ms(wchar_t * ms) {
  wchar_t * s;
  size_t cch;

  s = ms;
  while(*s) {
    log("%S\\0", s);
    StringCchLength(s, 512, &cch);
    s += cch + 1;
  }
}

int ms_to_csv_test(void) {
  wchar_t wbuf[512];
  int i;
  khm_int32 code = 0;
  size_t cbbuf;
  size_t cbr;
  size_t cbnull;

  for(i=0; i<n_strings; i++) {
    cbbuf = sizeof(wbuf);
    log("Multi string:[");
    print_ms(strings[i].ms);
    log("]->");
    code = multi_string_to_csv(NULL, &cbnull, strings[i].ms);
    code = multi_string_to_csv(wbuf, &cbbuf, strings[i].ms);
    if(code) {
      log(" returned %d\n", code);
      return code;
    }
    log("CSV[%S]", wbuf);
    if(wcscmp(wbuf, strings[i].csv)) {
      log(" MISMATCH!");
      return 1;
    }

    StringCbLength(wbuf, sizeof(wbuf), &cbr);
    cbr+= sizeof(wchar_t);

    if(cbr != cbbuf) {
      log(" Length mismatch");
      return 1;
    }

    if(cbnull != cbr) {
      log(" NULL length mismatch");
      return 1;
    }

    log("\n");
  }

  return code;
}

int csv_to_ms_test(void) {
  wchar_t wbuf[512];
  int i;
  khm_int32 code = 0;
  size_t cbbuf;
  size_t cbnull;

  for(i=0; i<n_strings; i++) {
    cbbuf = sizeof(wbuf);
    log("CSV:[%S]->", strings[i].csv);
    code = csv_to_multi_string(NULL, &cbnull, strings[i].csv);
    code = csv_to_multi_string(wbuf, &cbbuf, strings[i].csv);
    if(code) {
      log(" returned %d\n", code);
      return code;
    }
    log("MS[");
    print_ms(wbuf);
    log("]");

    if(cbnull != cbbuf) {
      log(" NULL length mismatch");
      return 1;
    }

    log("\n");

    log("  Byte length:%d\n", cbbuf);
  }

  return code;
}

int ms_append_test(void)
{
  wchar_t wbuf[512];
  size_t cbbuf;
  khm_int32 code;
  int i;

  for(i=0; i<n_strings; i++) {
    cbbuf = sizeof(wbuf);
    csv_to_multi_string(wbuf, &cbbuf, strings[i].csv);

    log("MS[");
    print_ms(wbuf);
    log("] + [foo]=[");
  
    cbbuf = sizeof(wbuf);
    code = multi_string_append(wbuf, &cbbuf, L"foo");

    if(code) {
      log(" returned %d\n", code);
      return code;
    }

    print_ms(wbuf);
    log("]\n");

    log("  byte length: %d\n", cbbuf);
  }
  return code;
}

int ms_delete_test(void)
{
  int code = 0;
  wchar_t wbuf[512];
  int i;
  size_t cbs;

  for(i=0; i<n_strings; i++) {
    cbs = sizeof(wbuf);
    csv_to_multi_string(wbuf, &cbs, strings[i].csv);

    log("MS[");
    print_ms(wbuf);
    log("] - [b]=[");

    log("cs:");
    code = multi_string_delete(wbuf, L"b", KHM_CASE_SENSITIVE);
    if(code) {
      log("ci:");
      code = multi_string_delete(wbuf, L"b", 0);
    }
    if(code) {
      log("pcs:");
      code = multi_string_delete(wbuf, L"b", KHM_CASE_SENSITIVE | KHM_PREFIX);
    }
    if(code) {
      log("pci:");
      code = multi_string_delete(wbuf, L"b", KHM_PREFIX);
    }

    if(!code)
      print_ms(wbuf);
    else
      log(" returned %d\n", code);

    log("]\n");
  }

  return code;
}

static nim_test tests[] = {
    {"ms2csv", "multi_string_to_csv() test", ms_to_csv_test},
    {"csv2ms", "csv_to_multi_string() test", csv_to_ms_test},
    {"msAppend", "multi_string_append() test", ms_append_test},
    {"msDel", "multi_string_delete() test", ms_delete_test}
};

nim_test_suite util_str_suite = {
    "UtilStr", "[util] Multi string and CSV tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
