

# Functions introduced in this section

The stages in **Part 3** are structured much the same as in **Part 2**, each
using their own versions of several functions. Things are a bit more complicated
this time, with 7 different functions implemented by each stage: 

* `filter_task()`
* `filter_frame()`
* `filter_sample()` 
* `calc_headroom()`
* `tx_frame()`
* `rx_frame_bfp()`
* `rx_and_merge_frame()`

`filter_task()` and `filter_sample()`, and `tx_frame()` fill largely the same
roles as in previous stages.

`filter_frame()` sits between `filter_task()` and `filter_sample()` on the call
stack. In BFP, we are typically calling operations which compute a whole vector
of values at once. `filter_frame()` fills that role for **Part 3**. In **Part
3**, `filter_sample()` is used by `filter_frame()` to compute individual output
elements, while `filter_frame()` is used by `filter_task()` to compute an entire
output frame at once.

`calc_headroom()` is a helper function which will be used to calculate the
headroom of our BFP vectors.

`tx_frame()` behaves much as it did in **Part 2**, except the output frame it is
given may not be using the desired output exponent, so `tx_frame()` will need to
coerce sample values into the correct output exponent before sending them to
tile 0.

In **Part 3** `rx_and_merge_frame()` and `rx_frame()` together perform a role
similar to that of `rx_frame()` in previous stages. `rx_frame()` behaves much
like it did before, but does not merge the frame into the sample history.
Instead it just reports the new frame's mantissas, exponent and headroom.

`rx_and_merge_frame()` is an unusual function which takes the BFP vector
representing the sample history as a parameter. It retrieves a BFP vector
representing the new frame with a call to `rx_frame()`, but then it must merge
the new frame of data into the sample history.

This is an unusual BFP operation because before we can merge the new frame of
data into the sample history, we have to ensure that both the new and old sample
data use the same exponent.








