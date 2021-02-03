#pragma once
#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "ast.hpp"
namespace ami {
class Parser {
    using u_ptr = std::shared_ptr<Expr>;
    std::vector<TokenHandler> m_Src;
    std::size_t m_Pos = 0;
    bool is_in_func_args{};

    TokenHandler m_Get() { return m_Src[m_Pos]; }
    bool not_eof() { return m_Pos < m_Src.size(); }
    bool m_IsDigit(const std::string& str) {
        for (auto& c : str)
            if (!std::isdigit(c) && c != '-') return false;
        return true;
    }
    std::string m_ParseFunctionBody() {
        std::string body{};
        while (not_eof() || m_Get().token == Tokens::Semicolon) {
            body += m_Get().value;
            m_Advance();
            if (m_Get().token == Tokens::Semicolon) {
                m_Advance();
                return body;
            }
        }
    }
    std::vector<u_ptr> m_ParseFunctionArgs() {
        std::vector<u_ptr> args{};
        if (m_Get().token == Tokens::Rparen) {
            m_Advance();
            return std::move(args);
        } else {
            while (not_eof() && (/*m_Get().token == Tokens::Comma ||*/
                                 m_Peek().token != Tokens::Rparen)) {
                if (m_Get().token == Tokens::Comma) {
                    m_Advance();
                } else {
                    args.push_back(std::move(m_ParseExpr()));
                }
            }
            return std::move(args);
        }
    }
    u_ptr m_ParseIdentAssign() {
        u_ptr value = std::move(m_ParseExpr());
        while (not_eof() && m_Get().token != Tokens::Semicolon) {
            m_Advance();
            value = std::move(m_ParseExpr());
        }
        return std::move(value);
    }
    std::string m_GetDigit() {
        std::string temp{};
        bool is_decimal{};
        bool contains_e{};
        while (not_eof() && (m_Get().token == Tokens::Digit ||
                             m_Get().token == Tokens::Dot ||
                             m_Get().token == Tokens::Delim ||
                             m_Get().token == Tokens::Edelim)) {
            if (m_Get().token == Tokens::Digit) {
                temp += m_Get().value;
            } else if (m_Get().token == Tokens::Dot) {
                if (is_decimal) {
                    m_Err();
                } else {
                    temp += m_Get().value;
                    is_decimal = true;
                }
            } else if (m_Get().token == Tokens::Edelim) {
                if (contains_e) {
                    m_Err();
                } else {
                    temp += m_Get().value;
                    contains_e = true;
                }
            }
            m_Advance();
        }
        return temp;
    }
    TokenHandler m_Peek() { return m_Src[m_Pos + 1]; }
    bool m_IsIdent(const std::string& str) {
        std::string::size_type count = 0;
        for (auto& c : str)
            if (std::isalpha(c) || c == '_') count++;
        return count == str.size();
    }
    void m_Advance(std::size_t x = 1) { m_Pos += x; }
    u_ptr m_ParseExpr() {
        u_ptr out = std::move(m_ParseTerm());
        while (not_eof() && (m_Get().token == Tokens::Plus ||
                             m_Get().token == Tokens::Minus)) {
            if (m_Get().token == Tokens::Plus) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(Op::Plus, std::move(out),
                                                     std::move(m_ParseTerm()));
            } else if (m_Get().token == Tokens::Minus) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(Op::Minus, std::move(out),
                                                     std::move(m_ParseTerm()));
            }
        }
        return std::move(out);
    }
    u_ptr m_ParseTerm() {
        u_ptr out = std::move(m_ParseSu());
        while (not_eof() && (m_Get().token == Tokens::Mult ||
                             m_Get().token == Tokens::Div)) {
            if (m_Get().token == Tokens::Mult) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(Op::Mult, std::move(out),
                                                     std::move(m_ParseSu()));
            } else if (m_Get().token == Tokens::Div) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(Op::Div, std::move(out),
                                                     std::move(m_ParseSu()));
            }
        }
        return std::move(out);
    }
    u_ptr m_ParseSu() {
        u_ptr out = std::move(m_ParseFactor());
        while (not_eof() &&
               (m_Get().token == Tokens::Pow || m_Get().token == Tokens::Mod)) {
            if (m_Get().token == Tokens::Pow) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(
                    Op::Pow, std::move(out), std::move(m_ParseFactor()));
            } else if (m_Get().token == Tokens::Mod) {
                m_Advance();
                out = std::make_shared<BinaryOpExpr>(
                    Op::Mod, std::move(out), std::move(m_ParseFactor()));
            }
        }
        return std::move(out);
    }
    u_ptr m_ParseFactor() {
        TokenHandler tok = m_Get();
        std::cout << "tok value: " << tok.value << '\n';
        if (tok.token == Tokens::Lparen) {
            m_Advance();
            auto out = std::move(m_ParseExpr());
            if (m_Get().token != Tokens::Rparen && !is_in_func_args) {
                m_Err();
            }
            m_Advance();
            return std::move(out);
        } else if (tok.token == Tokens::Digit) {
            if (m_IsDigit(tok.value)) {
                return std::make_shared<Number>(std::stod(m_GetDigit()));
            } else {
                m_Err();
            }
        } else if (tok.token == Tokens::Plus) {
            if (m_Pos > 0) {
                m_Advance();
                return std::make_shared<BinaryOpExpr>(
                    Op::Plus, std::move(m_ParseFactor()), nullptr);
            } else {
                m_Err();
            }
        } else if (tok.token == Tokens::Minus) {
            if (m_Pos > 0) {
                // we don't want to consider negative numbers as an
                // operation
                m_Advance();
                return std::make_shared<BinaryOpExpr>(
                    Op::Minus, std::move(m_ParseFactor()), nullptr);
            } else {
                m_Err();
            }
        } else if (tok.token == Tokens::Identifier) {
            if (m_Peek().token == Tokens::Assign) {
                m_Advance(2);  // skip the '='
                std::string name = tok.value;
                auto body = std::move(m_ParseIdentAssign());
                return std::make_shared<UserDefinedIdentifier>(name,
                                                               std::move(body));
            } else if (m_Peek().token == Tokens::Lparen) {
                m_Advance(2);  // skip the '('
                std::string name = tok.value;
                is_in_func_args = true;
                auto args = std::move(m_ParseFunctionArgs());
                return std::make_shared<FunctionCall>(name, std::move(args));
            } else {
                m_Advance();
                return std::make_shared<Identifier>(tok.value);
            }
        } else if (tok.token == Tokens::Comma) {
            if (is_in_func_args) m_Advance();
        } else if (tok.token == Tokens::Rparen) {
            if (is_in_func_args) {
                m_Advance();
                is_in_func_args = false;
            }
        } else {
            m_Err();
        }
    }
    void m_Err() { m_ThrowErr("Syntax Error"); }
    void m_ThrowErr(std::string_view msg) {
        std::stringstream ss;
        ss << msg << " at " << m_Pos << " invalid expression \""
           << m_Get().value << '"';
        throw std::runtime_error(ss.str());
    }

   public:
    explicit Parser(const std::vector<TokenHandler>& _tok) {
        if (_tok.size() > 0)
            m_Src = _tok;
        else
            throw std::runtime_error("invalid input");
    }
    u_ptr parse() {
        u_ptr out;
        while (not_eof()) {
            out = std::move(m_ParseExpr());
        }
        return std::move(out);
    }
};

}  // namespace ami