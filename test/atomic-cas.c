#include <stdio.h>
#include "internal/machine.h"

int main() {
    liteco_coroutine_t co;
    co.status = LITECO_TERMINATE;

    liteco_status_cas(&co, LITECO_RUNNING, LITECO_WAITING);

    printf("%d\n", co.status == LITECO_WAITING);

    return 0;
}
