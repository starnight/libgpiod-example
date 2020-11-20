# libgpiod event loop Example

This is a c++ libgpiod client using event loop for monitoring a GPIO line.

## Build
```
make
```

## Execute

```
./libgpiod-scan -n 10 -l 44
```

## Clean
```
make clean
```
### Notes

By default, program waits 10 seconds for an event to appear
