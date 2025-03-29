extern "C" {
    #include <ck_epoch.h>
    #include <ck_hs.h>
    #include <ck_malloc.h>
    #include <ck_pr.h>
}


int main() {
    ck_epoch_t epoch;
    ck_epoch_init(&epoch);
    return 0;
}
