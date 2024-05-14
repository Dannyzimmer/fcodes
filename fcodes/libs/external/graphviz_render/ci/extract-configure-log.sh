#!/usr/bin/awk -f

/will be compiled with the following:/,/^  criterion:     No/ {
    enable = 1;
}
{
    if (enable) {
        print $0;
    }
}
/^  criterion:     No/ {
    enable = 0;
}
