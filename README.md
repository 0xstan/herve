# herve
A type 2 hypervisor running as a lkm that will virtualise the running system. It supports EPT and can handle CPUID vmexits.

## Compilation

`make`

## Running the hypervisor
`insmod herve.ko`

```
$ echo 1 > /sys/kernel/herve/enable # enable
$ echo 0 > /sys/kernel/herve/enable # disable
```


