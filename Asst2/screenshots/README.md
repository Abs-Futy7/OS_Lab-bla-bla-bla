# Screenshots

Screenshots used by the Assignment 2 report:

| File | Command | Meaning |
|---|---|---|
| `asst2-02-sy2-lock.png` | `sy2` | Lock test completed successfully. |
| `asst2-03-sy3-cv.png` | `sy3` | Condition variable test completed successfully. |
| `asst2-04-palin.png` | `p /testbin/palin` | User program runs and writes output. |
| `asst2-05-argtest.png` | `p /testbin/argtest hello os161` | `argc/argv` argument passing works. |
| `asst2-06-filetest.png` | `mount sfs lhd0`, `p /testbin/filetest lhd0:testfile` | File creation/read/write/close/remove works on SFS. |

The `filetest` screenshot should show `mount sfs lhd0` and `Passed filetest.` because `filetest` must run on SFS, not emufs.
