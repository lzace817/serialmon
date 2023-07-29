# Serial hex monitor for arduino

Serial hex monitor based on 6502 hex monitor (wozmon) with some differences in the syntax.
It supports upper/lower case hexadecimal literals and aditional simbols for `examine-block` and `store`.

## Finding adresses

Interesting addresses to poke can be found inside the compiled elf with the toolchain's `readelf`.

### Example

 Lookup the address for `static uint16_t interval` (0x00800100 here):

``` Console
$ {toolchain}/avr-readelf {build-path}/serialmon.ino.elf -all | grep interval
    40: 00800100     2 OBJECT  LOCAL  DEFAULT    1 _ZZ4loopE8interval
```

 Ignore the MSD; only 4 LSD matter.
 Type `100: 50 0` and `return` on serial monitor

 The builtin led will flash faster.


# References

[Ben Eater's Video](https://www.youtube.com/watch?v=HlLCtjJzHVI) on Wozmon.
