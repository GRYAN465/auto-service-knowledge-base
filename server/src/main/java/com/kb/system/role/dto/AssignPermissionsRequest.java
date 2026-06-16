package com.kb.system.role.dto;

import lombok.Data;

import java.util.List;

@Data
public class AssignPermissionsRequest {

    private List<Long> permissionIds;
}
