package com.kb.recommend;

/** 用户画像分段：供向量加权融合。kind = immediate | recent | like | pin */
public record ProfileSegment(String text, String kind) {
}
