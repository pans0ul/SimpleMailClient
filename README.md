SimpleMailClient
=============================================

[中文](README_zh.md) | English

A mail client developed using the [SimpleMail](https://github.com/cutelyst/simple-mail) library.


## Features

- Read and send emails from specified text files
- Configure sending intervals and number of items to send
- Display scheduled sending tasks
- Support resume from breakpoint
- Encrypted email account credentials
- Static compilation support
- CLI and GUI versions available


## Quick Start

1. **Build**

    ```bash
    mkdir build && cd build && cmake ..
    cmake --build .
    ```

    **Static Build**
    ```bash
    mkdir build && cd build
    cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_EXECUTABLE=ON
    cmake --build .
    ```

    Note: Static compilation requires static Qt libraries
    
2. **Configuration**

    Three configuration methods are supported:

    - Config file: `--config mail_config.ini` (see docs/mail_config.ini for format)
    - Command line arguments: `--smtp-server`, `--sender-email`, `--sender-password`, `--recipient-email`
    - Header file: `../account_info.h` (compile-time configuration in inc_default.h)

3. **Run**

    ```bash
    Usage: ./SimpleMailClientCli [options] content interval count lines
    ```

    Example: `./SimpleMailClientCli ./content.txt 3s 3 3`
    
    This reads content.txt and sends 3 emails at 3-second intervals, reading 3 lines per email.


## Troubleshooting

### Intermittent "Mail rejected" errors are usually caused by:

**Email server rate limiting**
- Multiple sends in a short time may be temporarily rejected
- Recommended interval: at least 30 seconds to 1 minute

**Server load**
- SMTP server may reject requests when busy
- Load balancing policies may apply

**IP restrictions**
- Too many emails from the same IP in a short time
- May trigger anti-spam mechanisms

**Connection reuse issues**
- Frequent SMTP connection creation/destruction
- Server may limit connection count

**Solutions**

- Increase sending interval: Add delays between sends to avoid rapid consecutive sending
- Check network stability: Network instability can cause intermittent failures
- Switch email providers: Consider using more stable services like Gmail or QQ Mail


## Contributing

Contributions are welcome!


## License

This project (all files including the demos/examples) is licensed under the GNU LGPL, version 2.1+.
