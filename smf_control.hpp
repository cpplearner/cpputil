#pragma once

template<bool cond, class T>
struct default_copy_ctor_if;

template<bool cond, class T>
struct default_move_ctor_if;

template<bool cond, class T>
struct default_copy_assign_if;

template<bool cond, class T>
struct default_move_assign_if;

template<bool cond, class T>
struct delete_copy_ctor_if;

template<bool cond, class T>
struct delete_move_ctor_if;

template<bool cond, class T>
struct delete_copy_assign_if;

template<bool cond, class T>
struct delete_move_assign_if;
