#ifndef INTERACTIVE_SHELL_H
#define INTERACTIVE_SHELL_H

#include "AIRecommender.h"
#include "AdvancedSearch.h"
#include "BookService.h"
#include "BorrowService.h"
#include "CommunityManager.h"
#include "CreditSystem.h"
#include "DataAnalyzer.h"
#include "FileManager.h"
#include "MultiTerminalSync.h"
#include "NotificationCenter.h"
#include "ReaderService.h"
#include "ReportGenerator.h"
#include "ReservationSystem.h"
#include "SearchService.h"
#include "SecurityManager.h"

#include <optional>
#include <string>

struct CliSession {
    enum class Role { None, Reader, Admin };

    Role role{ Role::None };
    std::optional<std::string> readerId{};
    bool running{ true };

    void reset() {
        role = Role::None;
        readerId.reset();
    }
};

class InteractiveShell {
public:
    InteractiveShell(BookService& bookService,
        ReaderService& readerService,
        BorrowService& borrowService,
        SearchService& searchService,
        AdvancedSearch& advancedSearch,
        AIRecommender& recommender,
        ReservationSystem& reservationSystem,
        NotificationCenter& notificationCenter,
        CreditSystem& creditSystem,
        CommunityManager& communityManager,
        DataAnalyzer& dataAnalyzer,
        ReportGenerator& reportGenerator,
        FileManager& fileManager,
        SecurityManager& securityManager,
        MultiTerminalSync& multiSync);

    void run();

private:
    // ---- 顶层流程 ----
    void showWelcome() const;
    void handleLoginMenu();
    bool authenticateAdmin();
    bool ensureReaderLoggedIn();
    void mainMenu();

    // ---- 普通读者菜单 ----
    void readerMenu();
    void searchMenu();
    void borrowMenu();
    void reservationMenu();
    void recommenderMenu();
    void notificationMenu();

    // ---- 管理员主菜单 ----
    void adminMenu();
    void adminBookMenu();
    void adminReaderMenu();
    void adminBorrowMenu();
    void adminReservationMenu();
    void adminNotificationMenu();
    void adminCreditMenu();
    void adminInsightsMenu();
    void adminCommunityMenu();
    void adminMultiSyncMenu();
    void adminDataMenu();
    void adminSecurityMenu();

    // ---- 读者增改删 ----
    void performAdminRegisterReader();
    void performAdminUpdateReader();
    void performAdminDeleteReader();

    // ---- 普通读者功能实现 ----
    void performSearchByAuthor();
    void performSearchByTitle();
    void performSearchByKeyword();
    void performAdvancedSearch();
    void performBorrow();
    void performReturn();
    void performRenew();
    void performReservation();
    void performViewNotifications();
    void performViewRecommendations();

    // ---- 管理员功能实现 ----
    void performAdminAddBook();
    void performAdminDeleteBook();
    void performAdminAdjustStock();
    void performAdminEditBook();

    void performAdminBorrowHistoryByReader();
    void performAdminBorrowHistoryByBook();
    void performAdminBorrowStats();
    void performAdminBorrowOverdue();

    void performAdminReservationQueue();
    void performAdminReservationProcess();
    void performAdminReservationCancel();
    void performAdminReservationPriority();

    void performAdminSendDueReminder();
    void performAdminSendCustomNotification();

    void performAdminCreditView();
    void performAdminCreditAdjust();
    void performAdminCreditSuspend();
    void performAdminCreditRestore();

    void performAdminGenerateReport();
    void performAdminMonthlyReport();
    void performAdminBorrowTrend();
    void performAdminPredictPopular();

    void performAdminCommunityStats();
    void performAdminCreateThread();
    void performAdminListThreads();
    void performAdminCloseThread();

    void performAdminRegisterDevice();
    void performAdminListDevices();
    void performAdminScheduleSync();
    void performAdminCancelSync();

    void performAdminSaveData();
    void performAdminLoadData();
    void performAdminExportCSV();
    void performAdminExportJSON();
    void performAdminBackup();

    void performAdminAuditLogs();
    void performAdminRiskAssessment();

    // ---- 通用工具 ----
    void pauseAndWait() const;
    int readInt(const std::string& prompt, int min, int max) const;
    std::string readLine(const std::string& prompt) const;
    void printSeparator() const;

private:
    BookService& m_bookService;
    ReaderService& m_readerService;
    BorrowService& m_borrowService;
    SearchService& m_searchService;
    AdvancedSearch& m_advancedSearch;
    AIRecommender& m_recommender;
    ReservationSystem& m_reservationSystem;
    NotificationCenter& m_notificationCenter;
    CreditSystem& m_creditSystem;
    CommunityManager& m_communityManager;
    DataAnalyzer& m_dataAnalyzer;
    ReportGenerator& m_reportGenerator;
    FileManager& m_fileManager;
    SecurityManager& m_securityManager;
    MultiTerminalSync& m_multiSync;

    CliSession m_session;
};

#endif // INTERACTIVE_SHELL_H
