# libgpiod Scan Example

This is a libgpiod client scanning all GPIO controllers for a given GPIO line.

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

## Wiring Example

On a platform with multiple GPIO controllers, informs which one supports the given GPIO line
