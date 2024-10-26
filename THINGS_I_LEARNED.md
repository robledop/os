# qemu

We can tell qemu to log all interrupts by adding the following line to the qemu command:

```bash
-d int -D qemu.log
```

To grep the `qemu.log` file for lines that contain `v=`, which are the lines with
interrupt information, but exclude the interrupts 0x20, 0x21, and 0x80, which are
too common and fill up the whole log file, use the following command:

```bash
grep -P 'v=(?!20|21|80)' qemu.log
```

Here's an example line that the command will output:

```bash
176351: v=0e e=0007 i=0 cpl=3 IP=001b:00401eba pc=00401eba SP=0023:003fefd4 CR2=00000002
```

- `0176351`: is the count of interrupts so far
- `v=0e` is the interrupt vector, `0e` is the page fault interrupt
- `e` is the associated error code
- `i` is set to 1 if the exception comes from the hardware or an INT instruction
- `cpl` is the current privilege level, 0 = kernel mode, 3 = user mode
- `IP` and `PC` are the instruction pointer and program counter, i.e. address of the current instruction
- `SP` is the stack pointer

In this example, the error happened in user mode, at address `00401eba`, and the stack pointer is `003fefd4`.

I can then use `objdump` to disassemble the `sh` binary -which was the user-mode binary that was running at the time -
and find the instruction at address `00401eba`:

```bash
objdump -D -S sh | grep 401eba -A 50 -B 50
```

- `-A 50` and `-B 50` are used to show 50 lines after and before the address `00401eba`

In my case, it was in the `strcat` function:

<pre><code>
 char *strcat(char *dest, const char *src)
{
  401e90:	83 ec 10             	sub    $0x10,%esp
    char *d = dest;
  401e93:	8b 44 24 14          	mov    0x14(%esp),%eax
  401e97:	89 44 24 0c          	mov    %eax,0xc(%esp)
    while (*d != '\0') {
  401e9b:	eb 05                	jmp    401ea2 strcat+0x12
        d++;
  401e9d:	83 44 24 0c 01       	addl   $0x1,0xc(%esp)
    while (*d != '\0') {
  401ea2:	8b 44 24 0c          	mov    0xc(%esp),%eax
  401ea6:	0f b6 00             	movzbl (%eax),%eax
  401ea9:	84 c0                	test   %al,%al
  401eab:	75 f0                	jne    401e9d strcat+0xd
    }
    while (*src != '\0') {
  401ead:	eb 17                	jmp    401ec6 strcat+0x36
        *d = *src;
  401eaf:	8b 44 24 18          	mov    0x18(%esp),%eax
  401eb3:	0f b6 10             	movzbl (%eax),%edx
  401eb6:	8b 44 24 0c          	mov    0xc(%esp),%eax
  <mark>401eba</mark>:	88 10                	mov    %dl,(%eax)
        d++;
  401ebc:	83 44 24 0c 01       	addl   $0x1,0xc(%esp)
        src++;
  401ec1:	83 44 24 18 01       	addl   $0x1,0x18(%esp)
    while (*src != '\0') {
  401ec6:	8b 44 24 18          	mov    0x18(%esp),%eax
  401eca:	0f b6 00             	movzbl (%eax),%eax
  401ecd:	84 c0                	test   %al,%al
  401ecf:	75 de                	jne    401eaf strcat+0x1f
    }
    *d = '\0';
  401ed1:	8b 44 24 0c          	mov    0xc(%esp),%eax
  401ed5:	c6 00 00             	movb   $0x0,(%eax)
    return dest;
  401ed8:	8b 44 24 14          	mov    0x14(%esp),%eax
}   }
</code></pre>