#!/usr/bin/sh
# SPDX-License-Identifier: GPL-2.0
# helpers for dealing with atomics.tbl

#meta_in(meta, match)
meta_in()
{
	case "$1" in
	[$2]) return 0;;
	esac

	return 1
}

#meta_has_ret(meta)
meta_has_ret()
{
	meta_in "$1" "bBiIfFlR"
}

#meta_has_acquire(meta)
meta_has_acquire()
{
	meta_in "$1" "BFIlR"
}

#meta_has_release(meta)
meta_has_release()
{
	meta_in "$1" "BFIRs"
}

#meta_has_relaxed(meta)
meta_has_relaxed()
{
	meta_in "$1" "BFIR"
}

#meta_is_implicitly_relaxed(meta)
meta_is_implicitly_relaxed()
{
	meta_in "$1" "vls"
}

#find_template(tmpltype, pfx, name, sfx, order)
find_template()
{
	local tmpltype="$1"; shift
	local pfx="$1"; shift
	local name="$1"; shift
	local sfx="$1"; shift
	local order="$1"; shift

	local base=""
	local file=""

	# We may have fallbacks for a specific case (e.g. read_acquire()), or
	# an entire class, e.g. *inc*().
	#
	# Start at the most specific, and fall back to the most general. Once
	# we find a specific fallback, don't bother looking for more.
	for base in "${pfx}${name}${sfx}${order}" "${pfx}${name}${sfx}" "${name}"; do
		file="${ATOMICDIR}/${tmpltype}/${base}"

		if [ -f "${file}" ]; then
			printf "${file}"
			break
		fi
	done
}

#find_fallback_template(pfx, name, sfx, order)
find_fallback_template()
{
	find_template "fallbacks" "$@"
}

#find_kerneldoc_template(pfx, name, sfx, order)
find_kerneldoc_template()
{
	find_template "kerneldoc" "$@"
}

#gen_ret_type(meta, int)
gen_ret_type() {
	local meta="$1"; shift
	local int="$1"; shift

	case "${meta}" in
	[sv]) printf "void";;
	[bB]) printf "bool";;
	[aiIfFlR]) printf "${int}";;
	esac
}

#gen_ret_stmt(meta)
gen_ret_stmt()
{
	if meta_has_ret "${meta}"; then
		printf "return ";
	fi
}

# gen_param_name(arg)
gen_param_name()
{
	# strip off the leading 'c' for 'cv'
	local name="${1#c}"
	printf "${name#*:}"
}

# gen_param_type(arg, int, atomic)
gen_param_type()
{
	local type="${1%%:*}"; shift
	local int="$1"; shift
	local atomic="$1"; shift

	case "${type}" in
	i) type="${int} ";;
	p) type="${int} *";;
	v) type="${atomic}_t *";;
	cv) type="const ${atomic}_t *";;
	esac

	printf "${type}"
}

#gen_param(arg, int, atomic)
gen_param()
{
	local arg="$1"; shift
	local int="$1"; shift
	local atomic="$1"; shift
	local name="$(gen_param_name "${arg}")"
	local type="$(gen_param_type "${arg}" "${int}" "${atomic}")"

	printf "${type}${name}"
}

#gen_params(int, atomic, arg...)
gen_params()
{
	local int="$1"; shift
	local atomic="$1"; shift

	while [ "$#" -gt 0 ]; do
		gen_param "$1" "${int}" "${atomic}"
		[ "$#" -gt 1 ] && printf ", "
		shift;
	done
}

#gen_args(arg...)
gen_args()
{
	while [ "$#" -gt 0 ]; do
		printf "$(gen_param_name "$1")"
		[ "$#" -gt 1 ] && printf ", "
		shift;
	done
}

#gen_desc_return(meta)
gen_desc_return()
{
	local meta="$1"; shift

	case "${meta}" in
	[v])
		printf "Return: Nothing."
		;;
	[Ff])
		printf "Return: The original value of @v."
		;;
	[R])
		printf "Return: The updated value of @v."
		;;
	[l])
		printf "Return: The value of @v."
		;;
	esac
}

#gen_template_kerneldoc(template, class, meta, pfx, name, sfx, order, atomic, int, args...)
gen_template_kerneldoc()
{
	local template="$1"; shift
	local class="$1"; shift
	local meta="$1"; shift
	local pfx="$1"; shift
	local name="$1"; shift
	local sfx="$1"; shift
	local order="$1"; shift
	local atomic="$1"; shift
	local int="$1"; shift

	local atomicname="${atomic}_${pfx}${name}${sfx}${order}"

	local ret="$(gen_ret_type "${meta}" "${int}")"
	local retstmt="$(gen_ret_stmt "${meta}")"
	local params="$(gen_params "${int}" "${atomic}" "$@")"
	local args="$(gen_args "$@")"
	local desc_order=""
	local desc_instrumentation=""
	local desc_return=""

	if [ ! -z "${order}" ]; then
		desc_order="${order##_}"
	elif meta_is_implicitly_relaxed "${meta}"; then
		desc_order="relaxed"
	else
		desc_order="full"
	fi

	if [ -z "${class}" ]; then
		desc_noinstr="Unsafe to use in noinstr code; use raw_${atomicname}() there."
	else
		desc_noinstr="Safe to use in noinstr code; prefer ${atomicname}() elsewhere."
	fi

	desc_return="$(gen_desc_return "${meta}")"

	. ${template}
}

#gen_kerneldoc(class, meta, pfx, name, sfx, order, atomic, int, args...)
gen_kerneldoc()
{
	local class="$1"; shift
	local meta="$1"; shift
	local pfx="$1"; shift
	local name="$1"; shift
	local sfx="$1"; shift
	local order="$1"; shift

	local atomicname="${atomic}_${pfx}${name}${sfx}${order}"

	local tmpl="$(find_kerneldoc_template "${pfx}" "${name}" "${sfx}" "${order}")"
	if [ -z "${tmpl}" ]; then
		printf "/*\n"
		printf " * No kerneldoc available for ${class}${atomicname}\n"
		printf " */\n"
	else
	gen_template_kerneldoc "${tmpl}" "${class}" "${meta}" "${pfx}" "${name}" "${sfx}" "${order}" "$@"
	fi
}

#gen_proto_order_variants(meta, pfx, name, sfx, ...)
gen_proto_order_variants()
{
	local meta="$1"; shift
	local pfx="$1"; shift
	local name="$1"; shift
	local sfx="$1"; shift

	gen_proto_order_variant "${meta}" "${pfx}" "${name}" "${sfx}" "" "$@"

	if meta_has_acquire "${meta}"; then
		gen_proto_order_variant "${meta}" "${pfx}" "${name}" "${sfx}" "_acquire" "$@"
	fi
	if meta_has_release "${meta}"; then
		gen_proto_order_variant "${meta}" "${pfx}" "${name}" "${sfx}" "_release" "$@"
	fi
	if meta_has_relaxed "${meta}"; then
		gen_proto_order_variant "${meta}" "${pfx}" "${name}" "${sfx}" "_relaxed" "$@"
	fi
}

#gen_proto_variants(meta, name, ...)
gen_proto_variants()
{
	local meta="$1"; shift
	local name="$1"; shift
	local pfx=""
	local sfx=""

	meta_in "${meta}" "fF" && pfx="fetch_"
	meta_in "${meta}" "R" && sfx="_return"

	gen_proto_order_variants "${meta}" "${pfx}" "${name}" "${sfx}" "$@"
}

#gen_proto(meta, ...)
gen_proto() {
	local meta="$1"; shift
	for m in $(echo "${meta}" | grep -o .); do
		gen_proto_variants "${m}" "$@"
	done
}
