package com.kb.statistics.mapper;

import com.kb.statistics.dto.ArticleTotalsVO;
import com.kb.statistics.dto.CategoryRankVO;
import com.kb.statistics.dto.DateCountVO;
import com.kb.statistics.dto.HotKeywordVO;
import com.kb.statistics.dto.NameCountVO;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 统计聚合 Mapper：承载普通 lambda 查询无法表达的 GROUP BY / SUM 聚合，
 * 不绑定单一实体、不继承 BaseMapper，由 {@code @MapperScan("com.kb.**.mapper")} 扫描注册。
 *
 * <p>注意：原生 SQL 绕过 MyBatis-Plus 的 {@code @TableLogic} 逻辑删除，
 * 凡查 kb_article 必须显式带 {@code deleted = 0}（日志表 kb_search_log 无逻辑删，无需此条件）。
 */
public interface StatisticsMapper {

    /** 知识计数列总量（浏览/点赞/收藏/评论之和）；COALESCE 兜底空表返回 0。 */
    @Select("SELECT COALESCE(SUM(view_count), 0)     AS viewTotal, "
            + "COALESCE(SUM(like_count), 0)     AS likeTotal, "
            + "COALESCE(SUM(favorite_count), 0) AS favoriteTotal, "
            + "COALESCE(SUM(comment_count), 0)  AS commentTotal "
            + "FROM kb_article WHERE deleted = 0")
    ArticleTotalsVO articleTotals();

    /** 知识状态分布（DRAFT/PENDING_AUDIT/ONLINE/OFFLINE/REJECTED → 数量）。 */
    @Select("SELECT status AS name, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 GROUP BY status ORDER BY `count` DESC")
    List<NameCountVO> articleStatusDist();

    /** 知识类型分布（SCRIPT/TRAIN/PRODUCT/OFFICE → 数量）。 */
    @Select("SELECT knowledge_type AS name, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 AND knowledge_type IS NOT NULL "
            + "GROUP BY knowledge_type ORDER BY `count` DESC")
    List<NameCountVO> articleTypeDist();

    /**
     * 热门搜索词：按 keyword 聚合检索次数，并统计其中命中 0 结果的次数（内容缺口）。
     * {@code since} 为 null 时统计全部历史；否则只统计该时间点之后的检索。
     */
    @Select("<script>"
            + "SELECT keyword AS keyword, COUNT(*) AS searchCount, "
            + "SUM(CASE WHEN result_count = 0 THEN 1 ELSE 0 END) AS zeroCount "
            + "FROM kb_search_log "
            + "WHERE keyword IS NOT NULL AND keyword &lt;&gt; '' "
            + "<if test='since != null'>AND create_time &gt;= #{since} </if>"
            + "GROUP BY keyword ORDER BY searchCount DESC, zeroCount DESC "
            + "LIMIT #{limit}"
            + "</script>")
    List<HotKeywordVO> hotKeywords(@Param("since") LocalDateTime since, @Param("limit") int limit);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m-%d') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_view_log WHERE create_time >= #{since} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m-%d') ORDER BY `date`")
    List<DateCountVO> dailyViews(@Param("since") LocalDateTime since);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m-%d') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_search_log WHERE create_time >= #{since} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m-%d') ORDER BY `date`")
    List<DateCountVO> dailySearches(@Param("since") LocalDateTime since);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m-%d') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 AND create_time >= #{since} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m-%d') ORDER BY `date`")
    List<DateCountVO> dailyNewArticles(@Param("since") LocalDateTime since);

    @Select("SELECT DATE_FORMAT(online_time, '%Y-%m-%d') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 AND status = 'ONLINE' "
            + "AND online_time IS NOT NULL AND online_time >= #{since} "
            + "GROUP BY DATE_FORMAT(online_time, '%Y-%m-%d') ORDER BY `date`")
    List<DateCountVO> dailyOnlineArticles(@Param("since") LocalDateTime since);

    @Select("SELECT c.id AS categoryId, c.name AS categoryName, COUNT(a.id) AS articleCount, "
            + "COALESCE(SUM(a.view_count), 0) AS viewTotal "
            + "FROM kb_category c "
            + "INNER JOIN kb_article a ON a.category_id = c.id AND a.deleted = 0 "
            + "GROUP BY c.id, c.name ORDER BY articleCount DESC, viewTotal DESC LIMIT #{limit}")
    List<CategoryRankVO> categoryRank(@Param("limit") int limit);

    @Select("SELECT audit_status AS name, COUNT(*) AS `count` FROM kb_audit_record "
            + "WHERE audit_status IS NOT NULL AND audit_time IS NOT NULL "
            + "GROUP BY audit_status ORDER BY `count` DESC")
    List<NameCountVO> auditResultDist();

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m-%d') AS `date`, COUNT(*) AS `count` "
            + "FROM qa_session WHERE deleted = 0 AND create_time >= #{since} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m-%d') ORDER BY `date`")
    List<DateCountVO> dailyQaSessions(@Param("since") LocalDateTime since);

    @Select("SELECT COUNT(*) FROM qa_session WHERE deleted = 0 AND create_time >= #{since}")
    Long qaSessionCount(@Param("since") LocalDateTime since);

    @Select("SELECT COUNT(*) FROM qa_message WHERE create_time >= #{since}")
    Long qaMessageCount(@Param("since") LocalDateTime since);

    @Select("SELECT COUNT(*) FROM qa_feedback WHERE helpful = 1 AND create_time >= #{since}")
    Long qaHelpfulCount(@Param("since") LocalDateTime since);

    @Select("SELECT COUNT(*) FROM qa_feedback WHERE helpful = 0 AND create_time >= #{since}")
    Long qaUnhelpfulCount(@Param("since") LocalDateTime since);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_view_log WHERE create_time >= #{start} AND create_time < #{end} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m') ORDER BY `date`")
    List<DateCountVO> monthlyViews(@Param("start") LocalDateTime start, @Param("end") LocalDateTime end);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_search_log WHERE create_time >= #{start} AND create_time < #{end} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m') ORDER BY `date`")
    List<DateCountVO> monthlySearches(@Param("start") LocalDateTime start, @Param("end") LocalDateTime end);

    @Select("SELECT DATE_FORMAT(create_time, '%Y-%m') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 AND create_time >= #{start} AND create_time < #{end} "
            + "GROUP BY DATE_FORMAT(create_time, '%Y-%m') ORDER BY `date`")
    List<DateCountVO> monthlyNewArticles(@Param("start") LocalDateTime start, @Param("end") LocalDateTime end);

    @Select("SELECT DATE_FORMAT(online_time, '%Y-%m') AS `date`, COUNT(*) AS `count` "
            + "FROM kb_article WHERE deleted = 0 AND status = 'ONLINE' "
            + "AND online_time IS NOT NULL AND online_time >= #{start} AND online_time < #{end} "
            + "GROUP BY DATE_FORMAT(online_time, '%Y-%m') ORDER BY `date`")
    List<DateCountVO> monthlyOnlineArticles(@Param("start") LocalDateTime start, @Param("end") LocalDateTime end);
}
