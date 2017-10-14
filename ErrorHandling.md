# Error handling strategy for coroutine functions

## Http calls
- Throw exceptions only on socket errors
- Https status codes are returned, will not cause exceptions

## Soap calls
- Will throw exceptions on http errors
- Soap status is returned, soap errors will not cause exceptions