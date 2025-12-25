import argparse
import os
import re
import sys
import requests
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import selenium.common.exceptions as SE


def get_csa(query_name: str, page: int, save_dir: str):

    options = webdriver.EdgeOptions()
    options.add_argument("--headless=new")
    options.add_argument("--start-maximized")
    driver = webdriver.Edge(options=options)

    try:
        url = (
            f"https://www.shogi-extend.com/swars/search?query={query_name}&page={page}"
        )
        print(f"アクセス中: {url}")
        driver.get(url)

        wait = WebDriverWait(driver, 15)

        link_elems = wait.until(
            EC.presence_of_all_elements_located(
                (By.CSS_SELECTOR, "a.button.ShowButton.is-small")
            )
        )

        hrefs = [elem.get_attribute("href") for elem in link_elems]
        print(f"🔍 {len(hrefs)} 件の対局URLを取得")

        for i, battle_url in enumerate(hrefs, 1):
            match = re.search(r"battles/([^/]+)", battle_url)
            if not match:
                print("⚠ URL形式不正 → スキップ")
                continue

            battle_id = match.group(1)
            csa_url = (
                "https://www.shogi-extend.com/w/"
                + battle_id
                + ".csa?body_encode=UTF-8&disposition=attachment&format=csa&turn=31"
            )

            response = requests.get(csa_url, timeout=10)
            response.raise_for_status()  # エラーならここで例外

            filename = f"{save_dir}/{battle_id}.csa"
            with open(filename, "wb") as f:
                f.write(response.content)

            print(f"✅ 保存: {filename}")

    except SE.InvalidSessionIdException:
        print("⚠ セッション切断 → 強制終了")
        sys.exit(1)

    except Exception as e:
        print("✅ 全ての処理が終了しました")
        sys.exit(1)

    finally:
        try:
            driver.quit()
            print("ブラウザを閉じました。")
        except:
            pass  # 強制終了でもブラウザ閉じるだけ


# --------------------
# 実行部分
# --------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="プレーヤーの棋譜情報を取得")
    parser.add_argument(
        "--username",
        type=str,
        required=True,
    )
    args = parser.parse_args()
    username = args.username
    page = 1
    os.makedirs(f"./static/csa/{username}", exist_ok=True)

    while True:
        get_csa(username, page, f"./static/csa/{username}")
        page += 1
