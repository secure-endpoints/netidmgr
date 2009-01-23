
#include "tests.h"

static kconf_schema schema_kcdbconfig[] = {
{L"KCDB",KC_SPACE,0,L""},
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
{L"FooIdent",KC_ENDSPACE,0,L""},

{L"Identity",KC_ENDSPACE,0,L""},

{L"KCDB",KC_ENDSPACE,0,L""}
};


static int schema_load_test(void) {
    khm_handle csp_kcdb = NULL;

    CHECK(KHM_SUCCEEDED(khc_load_schema(NULL, schema_kcdbconfig)));
    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB", 0, &csp_kcdb)));
    CHECK(KHM_SUCCEEDED(khc_close_space(csp_kcdb)));
    CHECK(KHM_FAILED(khc_open_space(NULL, L"ADSFASDFASDF", 0, &csp_kcdb)));

    return 0;
}

static int shadow_config_test(void) {
    khm_handle csp_kcdb = NULL;
    khm_handle csp_ids = NULL;
    khm_handle csp_fooid = NULL;

    khm_int32 t;

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB", 0, &csp_kcdb)));
    CHECK(KHM_SUCCEEDED(khc_open_space(csp_kcdb, L"Identity", 0, &csp_ids)));
    CHECK(KHM_SUCCEEDED(khc_open_space(csp_ids, L"FooIdent", 0, &csp_fooid)));

    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"Sticky", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"Monitor", &t)));
    CHECK(t == 10);
    CHECK(KHM_FAILED(khc_read_int32(csp_fooid, L"RenewAtHalfLife", &t)));

    CHECK(KHM_SUCCEEDED(khc_close_space(csp_fooid)));
    CHECK(KHM_SUCCEEDED(khc_open_space(csp_ids, L"FooIdent", KCONF_FLAG_SHADOW, &csp_fooid)));

    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"Sticky", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"Monitor", &t)));
    CHECK(t == 10);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"RenewAtHalfLife", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp_fooid, L"AutoRenewThreshold", &t)));
    CHECK(t == 60);


    CHECK(KHM_SUCCEEDED(khc_close_space(csp_fooid)));
    CHECK(KHM_SUCCEEDED(khc_close_space(csp_ids)));
    CHECK(KHM_SUCCEEDED(khc_close_space(csp_kcdb)));

    return 0;
}

static int open_space_test(void) {
    khm_handle csp1 = NULL;
    khm_handle csp2 = NULL;
    khm_handle csp3 = NULL;
    khm_int32 t;

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, NULL, 0, &csp1)));
    CHECK(KHM_FAILED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_FAILED(khc_open_space(NULL, L"", 0, &csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB", 0, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent", 0, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent\\Blah", KCONF_FLAG_TRAILINGVALUE, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent", KCONF_FLAG_TRAILINGVALUE, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 4);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent", KCONF_FLAG_SHADOW, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent\\Something",
                                       KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent2", KCONF_FLAG_SHADOW, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 2);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity\\FooIdent2\\Something",
                                       KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp1)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"ID", &t)));
    CHECK(t == 2);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp1, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(NULL, L"KCDB\\Identity", 0, &csp1)));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent", 0, &csp2)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp2)));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent", KCONF_FLAG_TRAILINGVALUE, &csp2)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"ID", &t)));
    CHECK(t == 4);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp2)));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent",
                                       KCONF_FLAG_SHADOW | KCONF_FLAG_TRAILINGVALUE, &csp2)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"ID", &t)));
    CHECK(t == 4);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp2)));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent",
                                       KCONF_FLAG_SHADOW, &csp2)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp2)));

    CHECK(KHM_SUCCEEDED(khc_open_space(csp1, L"FooIdent2",
                                       KCONF_FLAG_SHADOW, &csp2)));
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"ID", &t)));
    CHECK(t == 2);
    CHECK(KHM_SUCCEEDED(khc_read_int32(csp2, L"AllowAutoRenew", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_close_space(csp2)));

    CHECK(KHM_FAILED(khc_open_space(NULL, L"KCDB\\Identity2", KCONF_FLAG_SHADOW, &csp2)));

    CHECK(KHM_SUCCEEDED(khc_close_space(csp1)));

    return 0;
}

static int direct_read_test(void)
{
    khm_int32 t;

    CHECK(KHM_SUCCEEDED(khc_read_int32(NULL, L"KCDB\\ID", &t)));
    CHECK(t == 1);
    CHECK(KHM_SUCCEEDED(khc_read_int32(NULL, L"KCDB\\Identity\\ID", &t)));
    CHECK(t == 4);
    CHECK(KHM_SUCCEEDED(khc_read_int32(NULL, L"KCDB\\Identity\\_Schema\\ID", &t)));
    CHECK(t == 2);
    CHECK(KHM_SUCCEEDED(khc_read_int32(NULL, L"KCDB\\Identity\\FooIdent\\ID", &t)));
    CHECK(t == 3);
    CHECK(KHM_FAILED(khc_read_int32(NULL, L"KCDB\\Identity\\FooIdent2\\ID", &t)));

    return 0;
}

static nim_test tests[] = {
    {"scmlod", "Schema load test", schema_load_test},
    {"shadow1", "Shadowed configuration space test", shadow_config_test},
    {"opensp", "khc_open_space() test", open_space_test},
    {"dirread", "Direct read test", direct_read_test},
};

nim_test_suite kconf_basic_suite = {
    "KconfBasic", "[KCONF] Basic Tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
