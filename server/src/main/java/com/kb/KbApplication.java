package com.kb;

import org.mybatis.spring.annotation.MapperScan;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

/**
 * 智能客服知识库系统 - 业务后端启动类。
 *
 * <p>分包见各 functional package：common / config / security / system / knowledge /
 * search / interaction / statistics / openapi / ai(二期) / realtime(二期)。
 *
 * <p>启用 @EnableScheduling 以支撑模块 10 实时辅助的定时推荐循环（RtRecommendService，5s）。
 */
@SpringBootApplication
@MapperScan("com.kb.**.mapper")
@EnableScheduling
public class KbApplication {

    public static void main(String[] args) {
        SpringApplication.run(KbApplication.class, args);
    }
}
