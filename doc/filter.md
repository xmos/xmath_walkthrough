
[Prev](sw_organization.md) | [Home](intro.md) | [Next](common.md)

# Digital FIR Filter

The filter to be implemented in each stage of this tutorial is a 1024-tap
digital FIR box filter in which the filter output is the simple arithmetic
average of the most recent 1024 input samples. The behavior of this filter is
not particularly interesting, but here we are concerned with the performance
details of each implementation, not the usefullness of the filter, so a simple
filter will do.

## Filter Definition

The output sample, $y[k]$, of an FIR filter at discrete time step $k$ given
a sequence of input samples $x[\,]$ is defined to be

$$
  y[k] = \sum_{n=0}^{N-1} x[k-n] \cdot b[n]
$$

where $N$ is the number of filter taps and $b[n]$ (with $n \in
\{0,1,2,...,N-1\}$) is a vector of filter coefficients.

The filter coefficient $b[n]$ has lag $n$. That is, the filter coefficient
$b[0]$ is always multiplied by the newest input sample at time $k$, $b[1]$ is
multiplied by the previous input sample, and so on.

The filter used throughout this tutorial has $N$ fixed at $1024$ taps. In our
case, because the filter is a simple box averaging filter, all $b[n]$ will be
the same value, namely $\frac{1}{N}$. So

$$
  b[n] = \frac{1}{N} \qquad \forall k\in \{0,1,2,...,N-1\}
$$
