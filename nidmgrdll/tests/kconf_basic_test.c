
#include "tests.h"
#include <wchar.h>

static kconf_schema schema_kcdbconfig[] = {
    {L"KCDBTest",KC_SPACE,0,L""},
    {L"ID",KC_INT32,1,L""},
    {L"IdentityProviderOrder",KC_STRING,(khm_int64) L"Krb5Ident",L""},
    {L"IdentSerial",KC_INT64,0,L""},
    {L"DefaultMonitor",KC_INT32,1,L""},
    {L"DefaultAllowAutoRenew",KC_INT32,1,L""},
    {L"DefaultSticky",KC_INT32,0,L""},
    {L"MaxThreshold",KC_INT32,86400,L""},
    {L"MinThreshold",KC_INT32,10,L""},

    {L"Identity",KC_SPACE,0,L""},

    {L"ID",KC_INT32,4,L""},

    {L"_Schema",KC_SPACE,0,L""},
    {L"ID",KC_INT32,2,L""},
    {L"Name",KC_STRING,(khm_int64) L"",L""},
    {L"IDProvider",KC_STRING,(khm_int64) L"",L""},
    {L"Sticky",KC_INT32,0,L""},
    {L"Monitor",KC_INT32,1,L""},
    {L"WarnThreshold",KC_INT32,900,L""},
    {L"AllowWarn",KC_INT32,1,L""},
    {L"CriticalThreshold",KC_INT32,60,L""},
    {L"AllowCritical",KC_INT32,1,L""},
    {L"AutoRenewThreshold",KC_INT32,60,L""},
    {L"AllowAutoRenew",KC_INT32,1,L""},
    {L"RenewAtHalfLife",KC_INT32,1,L""},
    {L"IconNormal",KC_STRING,(khm_int64) L"",L""},
    {L"IdentSerial",KC_INT64,0,L""},
    {L"_Schema",KC_ENDSPACE,0,L""},

    {L"FooIdent",KC_SPACE,0,L""},
    {L"ID",KC_INT32,3,L""},
    {L"Name",KC_STRING,(khm_int64) L"FooIdent",L""},
    {L"IDProvider",KC_STRING,(khm_int64) L"Some Identity Provider",L""},
    {L"Sticky",KC_INT32,1,L""},
    {L"Monitor",KC_INT32,10,L""},
    {L"IconNormal",KC_STRING,(khm_int64) L"Blah blah blah",L""},
    {L"IdentSerial",KC_INT64,32,L""},

    {L"32-bit Int",KC_INT32,10,L""},
    {L"64-bit Int",KC_INT64,11,L""},
    {L"Some string",KC_STRING,(khm_int64) L"Berry", L""},
    {L"Binary value",KC_BINARY, 0, L""},

    {L"FooIdent",KC_ENDSPACE,0,L""},

    {L"Identity",KC_ENDSPACE,0,L""},

    {L"KCDBTest",KC_ENDSPACE,0,L""}
};


#define IS(f) CHECK(KHM_SUCCEEDED(f))
#define ISNT(f) CHECK(KHM_FAILED(f))

static int schema_load_test(void) {
    khm_handle csp_kcdb = NULL;

    IS(khc_load_schema(NULL, schema_kcdbconfig));
    IS(khc_open_space(NULL, L"KCDBTest", 0, &csp_kcdb));
    IS(khc_close_space(csp_kcdb));
    ISNT(khc_open_space(NULL, L"ADSFASDFASDF", 0, &csp_kcdb));

    return 0;
}

static int shadow_config_test(void) {
    khm_handle csp_kcdb = NULL;
    khm_handle csp_ids = NULL;
    khm_handle csp_fooid = NULL;

    khm_int32 t;

    IS(khc_open_space(NULL, L"KCDBTest", 0, &csp_kcdb));
    IS(khc_open_space(csp_kcdb, L"Identity", 0, &csp_ids));
    IS(khc_open_space(csp_ids, L"FooIdent", 0, &csp_fooid));

    IS(khc_read_int32(csp_fooid, L"Sticky", &t));
    CHECK(t == 1);
    IS(khc_read_int32(csp_fooid, L"Monitor", &t));
    CHECK(t == 10);
    ISNT(khc_read_int32(csp_fooid, L"RenewAtHalfLife", &t));

    IS(khc_close_space(csp_fooid));
    IS(khc_open_space(csp_ids, L"FooIdent", KCONF_FLAG_SHADOW, &csp_fooid));

    IS(khc_read_int32(csp_fooid, L"Sticky", &t));
    CHECK(t == 1);
    IS(khc_read_int32(csp_fooid, L"Monitor", &t));
    CHECK(t == 10);
    IS(khc_read_int32(csp_fooid, L"RenewAtHalfLife", &t));
    CHECK(t == 1);
    IS(khc_read_int32(csp_fooid, L"AutoRenewThreshold", &t));
    CHECK(t == 60);


    IS(khc_close_space(csp_fooid));
    IS(khc_close_space(csp_ids));
    IS(khc_close_space(csp_kcdb));

    return 0;
}

static int open_space_test(void) {
    khm_handle csp1 = NULL;
    khm_handle csp2 = NULL;
    khm_handle csp3 = NULL;
    khm_int32 t;

    IS(khc_open_space(NULL, NULL, 0, &csp1));
    ISNT(khc_read_int32(csp1, L"ID", &t));
    IS(khc_close_space(csp1));

    ISNT(khc_open_space(NULL, L"", 0, &csp1));

    IS(khc_open_space(NULL, L"KCDBTest", 0, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent", 0, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 3);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent\\Blah", KCONF_FLAG_TRAILINGVALUE, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 3);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent", KCONF_FLAG_TRAILINGVALUE, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 4);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent", KCONF_FLAG_SHADOW, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 3);
    IS(khc_read_int32(csp1, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp1));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent\\Something",
                                       KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp1)));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 3);
    IS(khc_read_int32(csp1, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent2", KCONF_FLAG_SHADOW, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 2);
    IS(khc_read_int32(csp1, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent2\\Something",
                      KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp1));
    IS(khc_read_int32(csp1, L"ID", &t));
    CHECK(t == 2);
    IS(khc_read_int32(csp1, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest\\Identity", 0, &csp1));

    IS(khc_open_space(csp1, L"FooIdent", 0, &csp2));
    IS(khc_read_int32(csp2, L"ID", &t));
    CHECK(t == 3);
    IS(khc_close_space(csp2));

    IS(khc_open_space(csp1, L"FooIdent", KCONF_FLAG_TRAILINGVALUE, &csp2));
    IS(khc_read_int32(csp2, L"ID", &t));
    CHECK(t == 4);
    IS(khc_close_space(csp2));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent",
                                       KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp2)));
    IS(khc_read_int32(csp2, L"ID", &t));
    CHECK(t == 4);
    IS(khc_close_space(csp2));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent",
                                       KCONF_FLAG_SHADOW, &csp2)));
    IS(khc_read_int32(csp2, L"ID", &t));
    CHECK(t == 3);
    IS(khc_read_int32(csp2, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp2));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent2",
                                       KCONF_FLAG_SHADOW, &csp2)));
    IS(khc_read_int32(csp2, L"ID", &t));
    CHECK(t == 2);
    IS(khc_read_int32(csp2, L"AllowAutoRenew", &t));
    CHECK(t == 1);
    IS(khc_close_space(csp2));

    ISNT(khc_open_space(NULL, L"KCDBTest\\Identity2", KCONF_FLAG_SHADOW, &csp2));

    IS(khc_close_space(csp1));

    IS(khc_open_space(NULL, L"KCDBTest", 0, &csp1));
    IS(khc_open_space(csp1, L"Identity", 0, &csp2));
    IS(khc_open_space(csp2, L"_Schema", 0, &csp3));
    IS(khc_close_space(csp1));
    IS(khc_close_space(csp2));
    IS(khc_close_space(csp3));

    return 0;
}

static int direct_read_test(void)
{
    khm_int32 t;

    IS(khc_read_int32(NULL, L"KCDBTest\\ID", &t));
    CHECK(t == 1);
    IS(khc_read_int32(NULL, L"KCDBTest\\Identity\\ID", &t));
    CHECK(t == 4);
    IS(khc_read_int32(NULL, L"KCDBTest\\Identity\\_Schema\\ID", &t));
    CHECK(t == 2);
    IS(khc_read_int32(NULL, L"KCDBTest\\Identity\\FooIdent\\ID", &t));
    CHECK(t == 3);
    ISNT(khc_read_int32(NULL, L"KCDBTest\\Identity\\FooIdent2\\ID", &t));

    return 0;
}

static int reg_read_write_test(void)
{
    khm_int32 i32;
    khm_int64 i64;
    static const BYTE cubytes[16] = {
        8, 0, 12, 234, 39, 9, 0, 23, 59, 32, 29, 4, 29, 38, 57, 3
    };
    static const BYTE cmbytes[16] = {
        1, 2, 1, 28, 98, 2, 247, 2, 8, 8, 8, 8, 8, 5, 4, 3
    };
    BYTE bytes[16];
    wchar_t str[128];
    khm_size cb;

    khm_handle c_u = NULL;      /* user handle */
    khm_handle c_s = NULL;      /* schema handle */
    khm_handle c_m = NULL;      /* machien handle */

    /* Write some values in */
    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent",
                      KCONF_FLAG_SHADOW, &c_u));

    IS(khc_write_int32(c_u, L"32-bit Int", 20));
    IS(khc_write_int64(c_u, L"64-bit Int", 21));
    IS(khc_write_string(c_u, L"Some string", L"Straw"));
    IS(khc_write_binary(c_u, L"Binary value", cubytes, sizeof(cubytes)));

    /* Now read them back */
    IS(khc_read_int32(c_u, L"32-bit Int", &i32));
    CHECK(i32 == 20);
    IS(khc_read_int64(c_u, L"64-bit Int", &i64));
    CHECK(i64 == 21);
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Straw"));
    cb = sizeof(bytes);
    IS(khc_read_binary(c_u, L"Binary value", bytes, &cb));
    CHECK(cb == 16);
    CHECK(!memcmp(bytes, cubytes, sizeof(cubytes)));

    /* We didn't write 'Name'.  So it should read it from schema */
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Name", str, &cb));
    CHECK(cb == 18);
    CHECK(!wcscmp(str, L"FooIdent"));

    /* Now reopen a handle just for the schema.  It should only see
       values that were defined in the schema. */
    IS(khc_open_space(c_u, NULL, KCONF_FLAG_SCHEMA, &c_s));
    IS(khc_read_int32(c_s, L"32-bit Int", &i32));
    CHECK(i32 == 10);
    IS(khc_read_int64(c_s, L"64-bit Int", &i64));
    CHECK(i64 == 11);
    cb = sizeof(str);
    IS(khc_read_string(c_s, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Berry"));
    cb = sizeof(bytes);
    ISNT(khc_read_binary(c_s, L"Binary value", bytes, &cb));

    /* Open a handle for the machine layer.  Same deal as above. */
    IS(khc_open_space(c_u, NULL, KCONF_FLAG_MACHINE, &c_m));
    ISNT(khc_read_int32(c_m, L"32-bit Int", &i32));
    ISNT(khc_read_int64(c_m, L"64-bit Int", &i64));
    cb = sizeof(str);
    ISNT(khc_read_string(c_m, L"Some string", str, &cb));
    cb = sizeof(bytes);
    ISNT(khc_read_binary(c_m, L"Binary value", bytes, &cb));

    /* Write some values into the machine layer */
    IS(khc_write_int32(c_m, L"32-bit Int", 30));
    IS(khc_write_int64(c_m, L"64-bit Int", 31));
    IS(khc_write_string(c_m, L"Some string", L"Rasp"));
    IS(khc_write_binary(c_m, L"Binary value", cmbytes, sizeof(cmbytes)));

    /* And read them back */
    IS(khc_read_int32(c_m, L"32-bit Int", &i32));
    CHECK(i32 == 30);
    IS(khc_read_int64(c_m, L"64-bit Int", &i64));
    CHECK(i64 == 31);
    cb = sizeof(str);
    IS(khc_read_string(c_m, L"Some string", str, &cb));
    CHECK(cb == 10);
    CHECK(!wcscmp(str, L"Rasp"));
    cb = sizeof(bytes);
    IS(khc_read_binary(c_m, L"Binary value", bytes, &cb));
    CHECK(cb == 16);
    CHECK(!memcmp(bytes, cmbytes, sizeof(cmbytes)));

    /* The user space should see the same values as it saw earlier */
    IS(khc_read_int32(c_u, L"32-bit Int", &i32));
    CHECK(i32 == 20);
    IS(khc_read_int64(c_u, L"64-bit Int", &i64));
    CHECK(i64 == 21);
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Straw"));
    cb = sizeof(bytes);
    IS(khc_read_binary(c_u, L"Binary value", bytes, &cb));
    CHECK(cb == 16);
    CHECK(!memcmp(bytes, cubytes, sizeof(cubytes)));

    /* Remove the values */
    IS(khc_remove_value(c_u, L"32-bit Int", KCONF_FLAG_USER)); /* only user */
    IS(khc_remove_value(c_u, L"64-bit Int", KCONF_FLAG_MACHINE)); /* only machine */
    IS(khc_remove_value(c_u, L"Some string", 0)); /* all */
    IS(khc_remove_value(c_m, L"Binary value", 0)); /* only machine */
    ISNT(khc_remove_value(c_s, L"Some string", 0));

    /* Now look again */
    IS(khc_read_int32(c_u, L"32-bit Int", &i32));
    CHECK(i32 == 30);
    IS(khc_read_int64(c_u, L"64-bit Int", &i64));
    CHECK(i64 == 21);
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Berry"));
    cb = sizeof(bytes);
    IS(khc_read_binary(c_u, L"Binary value", bytes, &cb));
    CHECK(cb == 16);
    CHECK(!memcmp(bytes, cubytes, sizeof(cubytes)));

    /* Remove the machine layer */
    IS(khc_remove_space(c_m));

    /* Take one more look */
    IS(khc_read_int32(c_u, L"32-bit Int", &i32));
    CHECK(i32 == 10);
    IS(khc_read_int64(c_u, L"64-bit Int", &i64));
    CHECK(i64 == 21);
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Berry"));
    cb = sizeof(bytes);
    IS(khc_read_binary(c_u, L"Binary value", bytes, &cb));
    CHECK(cb == 16);
    CHECK(!memcmp(bytes, cubytes, sizeof(cubytes)));

    /* Nuke the user space */
    IS(khc_remove_space(c_u));

    /* Look */
    IS(khc_read_int32(c_u, L"32-bit Int", &i32));
    CHECK(i32 == 10);
    IS(khc_read_int64(c_u, L"64-bit Int", &i64));
    CHECK(i64 == 11);
    cb = sizeof(str);
    IS(khc_read_string(c_u, L"Some string", str, &cb));
    CHECK(cb == 12);
    CHECK(!wcscmp(str, L"Berry"));
    cb = sizeof(bytes);
    ISNT(khc_read_binary(c_u, L"Binary value", bytes, &cb));

    IS(khc_close_space(c_u));
    IS(khc_close_space(c_s));
    IS(khc_close_space(c_m));

    return 0;
}

static int reg_read_write_path_test(void)
{
    khm_handle c_u = NULL;
    khm_handle c_r = NULL;
    khm_int32 i32;

    IS(khc_open_space(NULL, L"KCDBTest\\Identity\\FooIdent",
                      KCONF_FLAG_SHADOW, &c_u));

    IS(khc_open_space(NULL, L"KCDBTest",
                      KCONF_FLAG_SHADOW, &c_r));

    IS(khc_read_int32(c_u, L"ID", &i32));
    CHECK(i32 == 3);

    IS(khc_write_int32(c_r, L"Identity\\FooIdent\\ID", 4));
    IS(khc_read_int32(c_u, L"ID", &i32));
    CHECK(i32 == 4);

    IS(khc_write_int32(NULL, L"KCDBTest\\Identity\\FooIdent\\ID", 5));
    IS(khc_read_int32(c_u, L"ID", &i32));
    CHECK(i32 == 5);

    IS(khc_remove_value(c_u, L"ID", 0));
    IS(khc_read_int32(c_u, L"ID", &i32));
    CHECK(i32 == 3);

    ISNT(khc_write_int32(NULL, L"KCDBTest\\Identity\\\\FooIdent\\ID", 5));

    IS(khc_close_space(c_u));
    IS(khc_close_space(c_r));
    return 0;
}

static nim_test tests[] = {
    {"scmlod", "Schema load test", schema_load_test},
    {"shadow1", "Shadowed configuration space test", shadow_config_test},
    {"opensp", "khc_open_space() test", open_space_test},
    {"dirread", "Direct read test", direct_read_test},
    {"rwtest", "Registry read/write test", reg_read_write_test},
    {"prwtest", "Registry read/write with path values", reg_read_write_path_test},
};

nim_test_suite kconf_basic_suite = {
    "KconfBasic", "[KCONF] Basic Tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
