#include <stdio.h>

int SOURCE_PUBLIC(void) {
    return 100;
}

int SOURCE_SECRET(void) {
    return 7;
}

void SINK_PUBLIC(int value) {
    printf("public output: %d\n", value);
}

void SINK_SECRET(int value) {
    printf("secret audit output: %d\n", value);
}

int add_fee(int value) {
    return value + 5;
}

void copy_value(int value, int *out) {
    *out = value;
}

int choose_public_code(int secret) {
    if (secret > 0) {
        return 1;
    }
    return 0;
}

int main(void) {
    int public_balance = SOURCE_PUBLIC();
    int secret_risk_score = SOURCE_SECRET();
    int copied_secret;

    int adjusted_public = add_fee(public_balance);
    SINK_PUBLIC(adjusted_public);

    int explicit_leak = add_fee(secret_risk_score);
    SINK_PUBLIC(explicit_leak);

    int implicit_leak = choose_public_code(secret_risk_score);
    SINK_PUBLIC(implicit_leak);

    copy_value(secret_risk_score, &copied_secret);
    SINK_SECRET(copied_secret);

    return 0;
}
