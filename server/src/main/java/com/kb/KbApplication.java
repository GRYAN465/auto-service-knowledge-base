package com.kb;

import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

/**
 * 智能客服知识库系统 - 业务后端启动类。
 *
 * <p>分包见各 functional package：common / config / security / system / knowledge /
 * search / interaction / statistics / openapi / ai(二期) / realtime(二期)。
 */
@SpringBootApplication
@MapperScan("com.kb.**.mapper")
public class KbApplication {

    public static void main(String[] args) {
        SpringApplication.run(KbApplication.class, args);
    }
}
