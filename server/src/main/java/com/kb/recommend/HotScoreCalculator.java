package com.kb.recommend;

import java.util.Collection;
import java.util.List;
import java.util.Map;

/** 知识热度 raw 分与归一化。 */
public final class HotScoreCalculator {

    private HotScoreCalculator() {
    }

    public static double rawScore(long viewCount, long likeCount, long commentCount,
                                  double viewWeight, double likeWeight, double commentWeight) {
        return viewWeight * Math.log(1 + Math.max(0, viewCount))
                + likeWeight * Math.log(1 + Math.max(0, likeCount))
                + commentWeight * Math.log(1 + Math.max(0, commentCount));
    }

    /** min-max 归一化到 [0,1]；空或全相等时返回 0.5。 */
    public static Map<Long, Double> normalize(Map<Long, Double> rawById) {
        if (rawById == null || rawById.isEmpty()) {
            return Map.of();
        }
        double min = rawById.values().stream().mapToDouble(Double::doubleValue).min().orElse(0);
        double max = rawById.values().stream().mapToDouble(Double::doubleValue).max().orElse(0);
        if (max - min < 1e-9) {
            return rawById.keySet().stream().collect(java.util.stream.Collectors.toMap(id -> id, id -> 0.5));
        }
        return rawById.entrySet().stream()
                .collect(java.util.stream.Collectors.toMap(
                        Map.Entry::getKey,
                        e -> (e.getValue() - min) / (max - min)));
    }

    public static double normalizeValue(double value, Collection<Double> pool) {
        if (pool == null || pool.isEmpty()) {
            return 0.5;
        }
        double min = pool.stream().mapToDouble(Double::doubleValue).min().orElse(0);
        double max = pool.stream().mapToDouble(Double::doubleValue).max().orElse(0);
        if (max - min < 1e-9) {
            return 0.5;
        }
        return (value - min) / (max - min);
    }
}
